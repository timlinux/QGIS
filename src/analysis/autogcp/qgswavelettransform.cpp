/***************************************************************************
qgswavelettransform.cpp - Extracts salient points from a raster image using the
 Haar wavelet transform
--------------------------------------
Date : 14-September-2010
Author : James Meyer
Copyright : (C) 2010 by C.J. Meyer
Email : jamesmeyerx@gmail.com

***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgswavelettransform.cpp 606 2010-10-29 09:13:39Z jamesm $ */

#include "qgswavelettransform.h"
#include <iostream>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <cassert>
#include "qgslogger.h"
#include <QFile>
#include <cfloat>
#include <algorithm>
#ifndef MAX
#define MAX(x, y) \
  ((x > y) ? x : y)
#endif

/*******************************************
 *    QgsWaveletTransform()
 *******************************************/
QgsWaveletTransform::QgsWaveletTransform( QgsRasterDataset* image ):
    mImage( image ),
    mRasterBand( DEFAULT_RASTERBAND ),
    mLevels( DEFAULT_LEVELS ),
    mThreshold( DEFAULT_THRESHOLD ),
    mLimit( SAL_LIMIT ),
    mBlackLimit( DEFAULT_BLACK_LIMIT ),
    mOrder( true ),
    mGridLists( NULL ),
    mMaxGcps( DEFAULT_MAX_FEATURES ),
    mRows( 1 ), mCols( 1 ),
    mExecuted( false ),
    mProgressFunction( NULL ),
    mProgressArg( NULL )

{
  mInitialized = initialize();
  mChipSize = ( int )pow( (double) 2, (double) mLevels );
}//QgsWaveletTransform::QgsWaveletTransform(QgsRasterDataset* image, unsigned int numPoints)

/*******************************************
 *    setFeatureSize
 *******************************************/
void QgsWaveletTransform::setFeatureSize( int xSize, int ySize )
{
  int fSize = MAX( xSize , ySize );
  //Calculate the number of decomposition levels based on feature size;
  mLevels = ( int ) ceil( log10(( double ) fSize ) / log10(( double ) 2 ) );
  mChipSize = ( int )pow( (double) 2, (double) mLevels );
}
/*******************************************
 *    initialize
 *******************************************/
bool QgsWaveletTransform::initialize()
{
  if ( mImage && !mImage->failed() )
  {

    mImage->imageSize( mRasterXSize, mRasterYSize );
    if ( mRasterXSize == 0 || mRasterYSize == 0 )
    {
      QgsLogger::debug( "Image has zero size", 1, "qgswavelettransform.cpp", "QgsWaveletTransform::initialize()" );
      return false;
    }
    return true;
  }
  else
  {
    QgsLogger::debug( "Invalid QgsRasterDataset object", 1, "qgswavelettransform.cpp", "QgsWaveletTransform::initialize()" );
    return false;
  }
}
/*******************************************
 *    ~QgsWaveletTransform
 *******************************************/
QgsWaveletTransform::~QgsWaveletTransform()
{
  if ( mGridLists )
  {
    delete [] mGridLists;
  }
}//QgsWaveletTransform::~QgsWaveletTransform()

/*******************************************
 *    execute()
 *******************************************/
bool QgsWaveletTransform::execute()
{
  QString message;
  if ( !mInitialized )
  {
    QgsLogger::debug( "Wavelet not initialized", 1, "qgswavelettransform.cpp", "QgsWaveletTransform::extractFeatures()" );
    return false;
  }

  QgsLogger::debug( QString( "Levels: " ) + QString::number( mLevels ) );

  if ( mGridLists )
  {
    delete [] mGridLists;
  }

  mGridLists = new PointsList[mRows * mCols];

  GDALDataset* gdalDS = mImage->gdalDataset();
  if ( mRasterBand < 1 || mRasterBand > gdalDS->GetRasterCount() )
  {
    return false;
  }
  GDALRasterBand* theBand = gdalDS->GetRasterBand( mRasterBand );
  gdalDS->GetMetadata();
  int xSize = theBand->GetXSize();
  int ySize = theBand->GetYSize();
  //Divide image into mChipSize grids
  int xIterations = ( int )( xSize / ( double ) mChipSize );
  int yIterations = ( int )( ySize / ( double ) mChipSize );

  int xoffset;
  int yoffset;
  QgsSalientPoint point;
  double* dbuf = new double[mChipSize * mChipSize];
  void* buffer = ( void* ) dbuf;
  double** cache = new double*[mLevels]; //extra address just to be able to dereference
  int cacheSize = ( mChipSize / 2 ) * ( mChipSize / 2 );
  QgsLogger::debug( message.sprintf( "Chip Cache Size = %d of %d elements \n", mLevels, cacheSize ) );
  for ( int i = 0; i < mLevels - 1; ++i )
  {
    cache[i] = new double[cacheSize];
  }
  QgsLogger::debug( message.sprintf( "Wavelet Iterations: %d x %d\n", xIterations, yIterations ) );

  double totalOperations = yIterations;
  double currentProgress = 0.0;
  //Main loop
  int nextXOffset;
  for ( int i = 0; i < yIterations; ++i )
  {
    yoffset = i * mChipSize;

    for ( int j = 0; j < xIterations; ++j )
    {
      xoffset = j * mChipSize;
      nextXOffset = ( j + 1 ) * mChipSize;
      CPLErr err = theBand->RasterIO( GF_Read, xoffset, yoffset, mChipSize, mChipSize, buffer, mChipSize, mChipSize, GDT_Float64, 0, 0 );
      if ( err == CE_Failure )
      {
        QgsLogger::debug( message.sprintf( "Wavelet RasterIO operation failed at %d, %d for chipsize %d", xoffset, yoffset, mChipSize ) );
        return false;
      }
      //Get the salient point in the area
      theBand->AdviseRead( nextXOffset, yoffset, mChipSize, mChipSize, mChipSize, mChipSize, GDT_Float64, 0 );
      if ( transform( dbuf, cache, mChipSize, mLevels, &point ) )
      {
        if ( point.saliency() > mThreshold )
        {
          //Convert from chip space to image space
          int x = ( int )point.x() + j * mChipSize;
          int y = ( int )point.y() + i * mChipSize;
          point.set( x, y );
          PointsList* theList = this->featureList( x, y );
          addPoint( theList, point, mOrder, mMaxGcps );
        }
      }

    }
    currentProgress = std::min( 1.0, ( i + 1 ) / totalOperations );
    if ( mProgressFunction != NULL )
    {
      mProgressFunction( currentProgress, mProgressArg );
    }
    /*std::cout<<".";
    std::cout.flush();*/
  }

  if ( mProgressFunction != NULL )
  {
    mProgressFunction( 1.0, mProgressArg );
  }

  for ( int i = 0; i < mLevels - 1; ++i )
  {
    delete [] cache[i];
  }
  delete [] cache;
  delete [] dbuf;
  mExecuted = true;
  return true;

}//QgsWaveletTransform::extractFeatures()

/*******************************************
 *    initialized()
 *******************************************/
bool QgsWaveletTransform::initialized()
{
  return mInitialized;
}
/*******************************************
 *    rasterBand()
 *******************************************/
int QgsWaveletTransform::rasterBand()
{
  return mRasterBand;

} //QgsWaveletTransform::rasterBand()

/*******************************************
 *    extractDistributed
 *******************************************/
QgsWaveletTransform::PointsList* QgsWaveletTransform::extractDistributed( int quantity ) const
{
  PointsList* pList = new PointsList();
  if ( !mExecuted )
  {
    return pList;
  }
  int numCells = mRows * mCols;
  int numPerCell = ( int )ceil(( double )quantity / numCells );

  for ( int i = 0; i < numCells; ++i )
  {
    PointsList::const_iterator it = mGridLists[i].begin();
    for ( int num = 0; num < numPerCell && it != mGridLists[i].end(); ++it, ++num )
    {
      addPoint( pList, ( *it ), mOrder, quantity );
    }
  }

  return pList;
}

QgsWaveletTransform::PointsList* QgsWaveletTransform::extract( int quantity, int column, int row, int offset ) const
{
  if ( !mExecuted
       || column < 0
       || column >= mCols
       || row < 0
       || row >= mRows )
  {
    return NULL;
  }
  PointsList* result = new PointsList();
  const PointsList& pList = mGridLists[row * mCols + column];
  if ( offset >= pList.size() )
  {
    return result;
  }
  PointsList::const_iterator it = pList.constBegin() + offset;
  for ( int count = 0; count < quantity && it != pList.constEnd(); ++it, ++count )
  {
    result->append( *it );
  }
  return result;
}
const QgsWaveletTransform::PointsList* QgsWaveletTransform::constList( unsigned int column, unsigned int row ) const
{
  if ( !mExecuted
       || column < (unsigned int) 0
       || column >= (unsigned int) mCols
       || row < (unsigned int) 0
       || row >= (unsigned int) mRows )
  {
    return NULL;
  }
  return &mGridLists[row * mCols + column];


}
/*******************************************
 *    transform
 *******************************************/
bool QgsWaveletTransform::transform( double* buffer, double** cache, int size, int levels, QgsSalientPoint* out )
{
  int counter = 0;
  /********************************************
   * Count the amount of black/no-data in the chip
   ********************************************/
  for ( int i = 0; i < size * size; i++ )
  {
    if ( FLT_EQUALS( buffer[i], 0.0 ) )
    {
      counter++;
    }
  }
  double average = (( double ) counter ) / ( size * size );

  if ( average > mBlackLimit )
  {
    return false;
  }
  int scale = 2;
  int dim = size / scale;
  double* read = buffer;
  int iCache = 0;
  double* write = cache[0];
  double avg;
  int xOff, yOff;
  int upDim;

  //Decompose
  while ( dim > 1 )
  {
    for ( int y = 0; y < dim; ++y )
    {
      for ( int x = 0; x < dim; ++x )
      {
        upDim = dim * 2;
        xOff = x * scale;
        yOff = y * scale;
        avg = (( read[yOff * upDim + xOff] + read[yOff * upDim + xOff + 1] ) / 2.0
               + ( read[( 1+yOff ) * upDim + xOff] + read[( 1+yOff ) * upDim + xOff + 1] ) / 2.0 ) / 2.0;
        write[y*dim + x] = avg;
      }
    }
    read = write;
    write = cache[++iCache];
    dim /= 2;
  }
  upDim = dim * 2;
  assert( read == cache[levels - 2] );
  int x = 0;
  int y = 0;
  double values[4];

  avg = (( read[0] + read[1] ) / 2.0
         + ( read[2] + read[3] ) / 2.0 ) / 2.0;


  iCache = levels - 1;

  double saliency = 0.0;

  char max;
  dim = 2;
  //Work backwards selecting highest variance
  for ( int lvl = 0; lvl < levels; ++lvl )
  {
    if ( lvl == levels - 1 )
    {
      read = buffer;
    }
    else
    {
      read = cache[levels - 2 - lvl];
    }
    xOff = x * 2;
    yOff = y * 2;
    values[0] = read[yOff * dim + xOff];
    values[1] = read[yOff * dim + xOff + 1];
    values[2] = read[( yOff+1 ) * dim + xOff];
    values[3] = read[( yOff+1 ) * dim + xOff + 1];
    max = findMaxDev( values, avg );
    saliency += abs( avg - values[max] );
    switch ( max )
    {
      case 0:
        x = xOff;
        y = yOff;
        break;
      case 1:
        x = xOff + 1;
        y = yOff;
        break;
      case 2:
        x = xOff;
        y = yOff + 1;
        break;
      case 3:
        x = xOff + 1;
        y = yOff + 1;
        break;
    }
    dim *= 2;
  }
  out->setSaliency( saliency );
  out->setX( x );
  out->setY( y );
  return true;

}

/*******************************************
 *    addPoint
 *******************************************/
void QgsWaveletTransform::addPoint( PointsList* list, QgsSalientPoint item , bool order, int limit )
{
  PointsList::iterator it;
  if ( order )
  {
    it = list->begin();
    for ( ; it != list->end() && item.saliency() < ( *it ).saliency(); it++ )
    {
      //Empty
    }
  }
  else
  {
    it = list->end();
    if ( limit > 0 && list->size() >= limit )
    {
      return;
    }
  }
  list->insert( it , item );
  while ( limit > 0 && list->size() > limit )
  {
    list->removeLast();
  }
}
/*******************************************
 *    featureList
 *******************************************/
QgsWaveletTransform::PointsList* QgsWaveletTransform::featureList( int pixel, int line )
{
  int x = ( int )((( double )pixel / mRasterXSize ) * mCols );
  int y = ( int )((( double )line / mRasterYSize ) * mRows );
  return &mGridLists[y * mCols + x];

}
void QgsWaveletTransform::setRasterBand( int band )
{
  if ( mImage )
  {
    if ( band < 1 )
    {
      mRasterBand = 1;
    }
    else if ( band > mImage->rasterBands() )
    {
      mRasterBand = mImage->rasterBands();
    }
    else
    {
      mRasterBand = band;
    }
  }
}

void QgsWaveletTransform::setProgressFunction( QgsProgressFunction pfnProgress )
{
  mProgressFunction = pfnProgress;
}

void QgsWaveletTransform::setProgressArgument( void* pProgressArg )
{
  mProgressArg = pProgressArg;
}

void QgsWaveletTransform::suggestDistribution( int nNumPoints )
{
  if ( nNumPoints <= 0 )
  {
    QgsLogger::debug( "Invalid number of points specified for distribution suggestion" );
    return;
  }
  int dimension = ( int ) ceil( sqrt( (double) nNumPoints ) );
  setDistribution( dimension, dimension );
}
