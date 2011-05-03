/***************************************************************************
qgsimageanalyzer.cpp - The ImageAnalyzer class is responsible for the physical
 analysis and processing of image data. This included detecting and extracting
 GCPs and GCP chips from images, as well as searching for these reference
 GCP chips on a raw image.
--------------------------------------
Date : 07-May-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsimageanalyzer.cpp 606 2010-10-29 09:13:39Z jamesm $ */

#include <algorithm>
#include "qgsimageanalyzer.h"
#include "qgswavelettransform.h"
#include "qgslogger.h"
#include "qgsimagechip.h"
#include "qgscrosscorrelator.h"
#include "qgscliptominmaxenhancement.h"
#include "qgscontrastenhancement.h"
#ifndef MIN
#define MIN(x, y) \
  ((x < y) ? x : y)
#endif

QgsImageAnalyzer::QgsImageAnalyzer( QgsRasterDataset* image ):
    mImageData( image ), mChipType( CHIPTYPE ),
    mChipBandCount( DEFAULT_CHIP_BANDS ),
    mBandNumber( DEFAULT_BAND ),
    mSearchFeatureRatio( SEARCH_FEATURE_MULTIPLIER ),
    mProgress( 0.0 ),
    mCorrelationThreshold( 0.85 )
{
  setChipSize( DEFAULT_CHIP_SIZE, DEFAULT_CHIP_SIZE );
}


//Doesn't own and therefore destroy mImageData.
QgsImageAnalyzer::~QgsImageAnalyzer()
{
}

QgsGcpSet* QgsImageAnalyzer::extractGcps( int amount )
{
  QgsLogger::debug( "Analyzer Starting GCP extraction" );
  if ( !mImageData || mImageData->failed() )
  {
    QgsLogger::debug( "Invalid image dataset" );
    return NULL;
  }
  setProgress( 0.0 );
  QgsWaveletTransform extractor( mImageData );
  extractor.setFeatureSize( mChipSize.width, mChipSize.height );
  //extractor.setDistribution(WAV_DISTRIB,WAV_DISTRIB);
  extractor.suggestDistribution( amount );
  extractor.setRasterBand( mBandNumber );
  QgsProgressFunction pfnProgress = (QgsProgressFunction) setWaveletProgress;
  extractor.setProgressFunction( pfnProgress );
  extractor.setProgressArgument(( void* ) this );
  extractor.execute();
  QgsWaveletTransform::PointsList* list = extractor.extractDistributed( amount );


  if ( list )
  {
    double extractProgress = 0.0;
    double totalOperations = ( double )list->size();

    QgsGcpSet* gcpset = new QgsGcpSet();
    QgsWaveletTransform::PointsList::const_iterator it = list->begin();
    for ( int i = 0; it != list->end(); ++it, ++i )
    {
      QgsSalientPoint feature = ( *it );
      int pixX = ( int )feature.x();
      int pixY = ( int )feature.y();
      ////NB: assign pixel values before extracting because extraction changes point
      QgsRasterDataset* chip = extractChip( feature, mChipSize.width, mChipSize.height, mChipBandCount, mChipType );
      if ( !chip )
      {
        QgsLogger::debug( "Chip extraction failed", 1, "qgsimageanalyzer.cpp", "QgsImageAnalyzer::extractGcps()" );
      }
      QgsGcp* gcp = new QgsGcp();
      double geoX, geoY;

      mImageData->transformCoordinate( pixX, pixY, geoX, geoY, QgsRasterDataset::TM_PIXELGEO );
      gcp->setRefX( geoX );
      gcp->setRefY( geoY );
      gcp->setRawX( pixX );
      gcp->setRawY( pixY );
      gcp->setGcpChip( chip );

      gcpset->addGcp( gcp );
      extractProgress = (( i + 1 ) / totalOperations ) * ( 1.0 - WAVELET_PART );
      setProgress( extractProgress );
    }
    setProgress( 1.0 );
    QgsLogger::debug( "GCP Extraction Complete" );
    delete list;

    return gcpset;
  }
  else
  {
    QgsLogger::debug( "NULL list returned by Wavelet", 1, "qgsimageanalyzer.cpp", "QgsImageAnalyzer::extractGcps()" );

    return NULL;
  }

}

//Matches each incoming GCP to point that most resembles it on the raw image
//If no match is found, the raw coordinates and correlation coefficient are set to -1
QgsGcpSet* QgsImageAnalyzer::matchGcps( QgsGcpSet* gcpSet )
{
  if ( gcpSet->size() > 0 )
  {

    typedef QList<QgsGcp*> GcpList;
    GcpList& refList = gcpSet->list();
    GcpList::iterator it = refList.begin();
    QgsGcp* firstGcp = ( *it );
    QgsRasterDataset* firstChip = firstGcp->gcpChip();
    int featureSize = firstChip->imageXSize();
    setProgress( 0.0 );
    QgsWaveletTransform* wavelet = new QgsWaveletTransform( mImageData );
    wavelet->setDistribution( WAV_DISTRIB, WAV_DISTRIB );
    int searchFeatureSize = ( int )featureSize * mSearchFeatureRatio;
    wavelet->setFeatureSize( searchFeatureSize, searchFeatureSize );
    wavelet->setRasterBand( mBandNumber );
    wavelet->execute();

    QgsGrid* grid = new QgsGrid( featureSize * SEARCH_WINDOW_MULTIPLIER , mImageData );
    int distribColumns, distribRows;
    wavelet->distribution( distribColumns, distribRows );
    for ( int row = 0; row < distribRows; ++row )
    {
      for ( int col = 0; col < distribColumns; ++col )
      {
        const QgsWaveletTransform::PointsList* list = wavelet->constList( col, row );
        grid->fillGrid( list );
      }
    }
    delete wavelet;
    typedef QList<QgsPoint> PointsList;

    QgsLogger::debug( "Starting Cross Correlation" );
    //int counter = 0;
    for ( ; it != refList.end(); )
    {
#ifdef AUTOGCP_DEBUG
      counter ++;
      if ( counter == 8 )
      {
        int debughere = 1;
      }
      printf( "****************************\n" );
      printf( "Reference Point: (%f,%f)\n", gcp->refX(), gcp->refY() );
#endif
      QgsGcp* gcp = ( *it );
      PointsList* listPtr = grid->gridList( gcp );
      if ( listPtr != NULL )
      {
        PointsList& rawList = *listPtr;
        if ( rawList.size() > 0 )
        {
          PointsList::const_iterator bestIt = rawList.begin();
          double bestCorrelation;
          QgsPoint extractPoint = *bestIt;
          int pixelX = extractPoint.x(),
                       pixelY = extractPoint.y();
          QgsRasterDataset* rawChip = extractChip( extractPoint, featureSize, featureSize, -1, UNKNOWN_TYPE );
          QgsCrossCorrelator correlator( gcp->gcpChip(), rawChip );
          bestCorrelation = correlator.correlationValue();
          //printf("i-2: %d\n", i);
          PointsList::const_iterator rawIt = rawList.begin() + 1; //Already did first one
          double geoX, geoY;

          mImageData->transformCoordinate( pixelX, pixelY, geoX, geoY, QgsRasterDataset::TM_PIXELGEO );
#ifdef AUTOGCP_DEBUG
          printf( "Candidate: (%f, %f)\n", geoX, geoY );
#endif
          for ( ; rawIt != rawList.end(); ++rawIt )
          {
            extractPoint = ( *rawIt );
            pixelX = extractPoint.x();
            pixelY = extractPoint.y();
            mImageData->transformCoordinate( pixelX, pixelY, geoX, geoY, QgsRasterDataset::TM_PIXELGEO );
#ifdef AUTOGCP_DEBUG
            printf( "Candidate: (%f, %f)\n", geoX, geoY );
#endif
            rawChip = extractChip( extractPoint, featureSize, featureSize, -1, UNKNOWN_TYPE );
            QgsCrossCorrelator correlator( gcp->gcpChip(), rawChip );
            double correlationValue = correlator.correlationValue();
            if ( correlationValue > bestCorrelation )
            {
              bestIt = rawIt;
              bestCorrelation = correlationValue;
            }
          }
          //printf("i-3: %d\n", i);
          delete rawChip;
          if ( bestCorrelation > mCorrelationThreshold )
          {
            double geoX, geoY;
            const QgsPoint& p = ( *bestIt );
            int pixelX = ( int )p.x(),
                         pixelY = ( int )p.y();
            mImageData->transformCoordinate( pixelX, pixelY, geoX, geoY, QgsRasterDataset::TM_PIXELGEO );
#ifdef AUTOGCP_DEBUG
            if ( geoX != gcp->refX() || geoY != gcp->refY() )
            {
              printf( "Bad match: %f\n", bestCorrelation );
            }
#endif
            gcp->setRawX( geoX );
            gcp->setRawY( geoY );
            gcp->setCorrelationCoefficient( bestCorrelation );
            ++it;
          }
          else
          {
            it = refList.erase( it );
            delete gcp;
          }
          //printf("i-4: %d\n", i);
        }
        else
        {//Not found.
          //Remove from gcp set
          it = refList.erase( it );
          delete gcp;
        }
      }
      else
      {
        it = refList.erase( it );
        delete gcp;
      }
    }
    setProgress( 1.0 );
    QgsLogger::debug( "GCP Cross-Correlation Complete" );
    delete grid;
    return gcpSet;
  }
  else
  {
    return NULL;
  }

}
QgsGcpSet* QgsImageAnalyzer::bruteMatchGcps( QgsGcpSet* gcpSet )
{
  if ( !gcpSet || gcpSet->size() == 0 )
  {
    QgsLogger::debug( "Empty or invalid QgsGcpSet given" );
    return NULL;
  }
  if ( !mImageData->reopen() )
  {
    QgsLogger::debug( "Failed to reopen QgsRasterDataset" );
    return NULL;
  }
  typedef QList<QgsGcp*> GcpList;
  GcpList& list = gcpSet->list();
  setProgress( 0.0 );
  double totalOperations = list.size();
  double curProgress = 0.0;
  GcpList::iterator it = list.begin();
  for ( int i = 0; it != list.end(); ++i )
  {
    QgsGcp* gcp = ( *it );
    int pixel, line;
    //Convert Geo to pixel coords

    double geoX = gcp->refX(),
                  geoY = gcp->refY();
    mImageData->transformCoordinate( pixel, line, geoX, geoY, QgsRasterDataset::TM_GEOPIXEL );

    QgsRasterDataset* refChip = gcp->gcpChip();
    int nChipWidth = refChip->imageXSize();
    int nChipHeight = refChip->imageYSize();
    //create search window on raw image.
    int nSearchX = ( int ) nChipWidth * SEARCH_WINDOW_MULTIPLIER;
    int nSearchY = ( int ) nChipHeight * SEARCH_WINDOW_MULTIPLIER;
    int nWindowStartX = pixel - ( nSearchX / 2 );
    int nWindowStartY = line - ( nSearchY / 2 );

    if ( nWindowStartX < 0 )
    {
      nWindowStartX = 0;
    }
    if ( nWindowStartY < 0 )
    {
      nWindowStartY = 0;
    }
    if ( nWindowStartX + nSearchX >= mImageData->imageXSize() )
    {
      nSearchX = mImageData->imageXSize() - nWindowStartX;
    }
    if ( nWindowStartY + nSearchY >= mImageData->imageYSize() )
    {
      nSearchY = mImageData->imageYSize() - nWindowStartY;
    }
    //TODO: iterate through each raw pixel of search window, performing cross-correlation.
    size_t chipAllocSize = nChipWidth * nChipHeight * sizeof( double );
    int mCorrelationBand = 1;

    //Read src data from chip
    void* srcBuffer = VSIMalloc( chipAllocSize );
    void* destBuffer = VSIMalloc( chipAllocSize );
    GDALDataset* refDs = refChip->gdalDataset();
    GDALDataset* rawDs = mImageData->gdalDataset();

    GDALRasterBand* rawBand = rawDs->GetRasterBand( mCorrelationBand );
    GDALRasterBand* refBand = refDs->GetRasterBand( mCorrelationBand );
    refBand->RasterIO( GF_Read, 0, 0, nChipWidth, nChipHeight, srcBuffer, nChipWidth, nChipHeight, GDT_Float64, 0, 0 );
    int bestX = 0,
                bestY = 0;
    double bestCorrelation = 0.0;
    double correlationCoefficient;
    double progressInterval = ( 1.0 / totalOperations ) / ( nSearchY - nChipHeight );
    bool bFound = false;
    for ( int y = nWindowStartY; y < nWindowStartY + nSearchY - nChipHeight; ++y )
    {
      for ( int x = nWindowStartX; x < nWindowStartX + nSearchX - nChipWidth; ++x )
      {
        //Read destination data from image
        rawBand->RasterIO( GF_Read, x, y, nChipWidth, nChipHeight, destBuffer, nChipWidth, nChipHeight, GDT_Float64, 0, 0 );

        correlationCoefficient = QgsCrossCorrelator::correlationCoefficent(( double* ) srcBuffer, ( double* ) destBuffer, nChipWidth, nChipHeight );

        if ( correlationCoefficient > bestCorrelation )
        {
          bestCorrelation = correlationCoefficient;
          bestX = x;
          bestY = y;
          if ( bestCorrelation >= MATCH_ASSUMED )
          {
            bFound = true;
            break;
          }
        }
      }
      curProgress += progressInterval;
      setProgress( curProgress );
      if ( bFound )
      {
        break;
      }
    }
    VSIFree( srcBuffer );
    VSIFree( destBuffer );
    if ( bestCorrelation > mCorrelationThreshold )
    {
      double rawX, rawY;
      bestX += nChipWidth / 2;
      bestY +=  nChipHeight / 2;
      mImageData->transformCoordinate( bestX, bestY, rawX, rawY, QgsRasterDataset::TM_PIXELGEO );
      gcp->setRawX( rawX );
      gcp->setRawY( rawY );
      gcp->setCorrelationCoefficient( bestCorrelation );
      ++it;
    }
    else
    {
      it = list.erase( it );
      delete gcp;
    }
    curProgress = ( i + 1 ) / totalOperations;

  }
  setProgress( 1.0 );
  mImageData->close();
  QgsLogger::debug( "GCP Cross-Correlation Complete" );
  return gcpSet;
}

QgsRasterDataset* QgsImageAnalyzer::extractChip( QgsPoint& sourcePoint, int xPixels, int yLines,  int bands, GDALDataType destType, QgsRasterDataset* dest )
{
  int chipXSize = xPixels;
  int chipYSize = yLines;
  int startX = ( int )sourcePoint.x();
  int startY = ( int )sourcePoint.y();
  int offsetX = startX - chipXSize / 2;
  int offsetY = startY - chipYSize / 2;
  GDALDataset* source = mImageData->gdalDataset();
  if ( offsetX < 0 )
  {
    offsetX = 0;
  }
  if ( offsetY < 0 )
  {
    offsetY = 0;
  }

  if ( offsetX + chipXSize > source->GetRasterXSize() )
  {
    offsetX = source->GetRasterXSize() - chipXSize;
  }
  if ( offsetY + chipYSize > source->GetRasterYSize() )
  {
    offsetY = source->GetRasterYSize() - chipYSize;
  }

  //If offsets below 0 now, it means chipsize is larger than source image.
  if ( offsetX < 0 )
  {
    offsetX = 0;
    chipXSize = source->GetRasterXSize();
  }
  if ( offsetY < 0 )
  {
    offsetY = 0;
    chipYSize = source->GetRasterYSize();
  }
  //printf("Internal Offsets: %d, %d\n", offsetX, offsetY);
  sourcePoint.setX( offsetX );
  sourcePoint.setY( offsetY );
  //printf("Start X = %d, y = %d\n", startX, startY);

  return this->extractArea( offsetX, offsetY, chipXSize, chipYSize, bands, destType, dest );


}
QgsRasterDataset* QgsImageAnalyzer::extractArea( int offsetX, int offsetY, int width, int height, int bands, GDALDataType destType, QgsRasterDataset* dest )
{
  GDALDataset* source = mImageData->gdalDataset();
  GDALDataType sourceType = source->GetRasterBand( 1 )->GetRasterDataType();

  if ( offsetX < 0
       || offsetY < 0 )
  {
    return NULL;
  }

  if ( offsetX + width > source->GetRasterXSize() )
  {
    width = source->GetRasterXSize() - offsetX;
  }
  if ( offsetY + height > source->GetRasterYSize() )
  {
    height = source->GetRasterYSize() - offsetY;
  }
  if ( destType == UNKNOWN_TYPE )
  {
    destType = sourceType;
  }
  QgsRasterDataset* theChip = NULL;
  if ( 0 >= bands )
  {
    bands = mImageData->rasterBands();
  }
  else if ( mImageData->rasterBands() < bands )
  {
    bands = mImageData->rasterBands();
  }
  //printf("Bands to extract: %d\n", bands);
  // printf("DataType: %d\n", destType);
  // printf("Source Image: %p\n", source);


  if ( dest )
  {
    theChip = dest;
    if ( width < theChip->imageXSize() || height < theChip->imageYSize() )
    {
      QgsLogger::debug( "Destination dataset is too small for target chip extraction", 1, "qgsimageanalyzer.cpp", "QgsImageAnalyzer::extractChip()" );
      return NULL;
    }
  }
  else
  {
    theChip = QgsImageChip::createImageChip( width, height, destType, bands );
    if ( !theChip )
    {
      QgsLogger::debug( "Failed to create GCP chip", 1, "qgsimageanalyzer.cpp", "QgsImageAnalyzer::extractChip()" );
      return NULL;
    }
  }

  GDALDataset* destDS = theChip->gdalDataset();

  if ( !source )
  {
    QgsLogger::debug( "Invalid source. Cannot extract chips", 1, "qgsimageanalyzer.cpp", "QgsImageAnalyzer::extractChip()" );
    return NULL;
  }

  double geoTrans[6];
  source->GetGeoTransform( geoTrans );
  //Geo X-offset
  geoTrans[0] += offsetX * geoTrans[1] + offsetY * geoTrans[2];
  //Geo Y-offset
  geoTrans[3] += offsetX * geoTrans[4] + offsetY * geoTrans[5];
  //meters/pixel Stays the same.
  destDS->SetGeoTransform( geoTrans );

  //  printf("Offset: %d, %d\n", offsetX, offsetY);
  //  printf("Size: %d, %d\n", chipXSize, chipYSize);
  //Temporary buffer with uniform datatype
  void* buffer = VSIMalloc( width * height * sizeof( double ) );
  //rintf("Buffer size: %d\n", chipXSize * chipYSize * sizeof(double));
  try
  {
    for ( int i = 1; i <= bands; i++ )
    {
      GDALRasterBand* srcBand = source->GetRasterBand( i );
      GDALRasterBand* destBand = destDS->GetRasterBand( i );
      char msg[100];
      CPLErr result;
      char format[] = "Failed to %s image at band %d";
      if ( CE_Failure == ( result = srcBand->RasterIO( GF_Read, offsetX, offsetY, width, height, buffer, width, height, GDT_Float64, 0, 0 ) ) )
      {
        sprintf( msg, format, "read from source", i );
        throw QString( msg );
        break;
      }

      double min = srcBand->GetMinimum();
      double max = srcBand->GetMaximum();
      if ( min < 0 || max > 255 )
      {
        int totalIndexes = width * height;
        double *bufferValues = ( double* ) buffer;
        for ( int j = 0; j < totalIndexes; ++j )
        {
          bufferValues[j] = (( bufferValues[j] - min ) / ( max - min ) ) * 255; //Scale the values to 0 -255
        }
      }




      /*double *bufferValues = (double*) buffer;
      int totalIndexes = width * height;
      bool devide = false;
      for ( int j = 0; j < totalIndexes; ++j )
      {
        if(bufferValues[j] > 255)
      {
      devide = true;
      break;
      }
      }
      if(devide)
      {
      for ( int j = 0; j < totalIndexes; ++j )
      {
      bufferValues[j] /= 64;
      }
      }*/
      //printf("RESULT:%d\n",result);
      if ( CE_Failure == destBand->RasterIO( GF_Write, 0, 0, width, height, buffer, width, height, GDT_Float64, 0, 0 ) )
      {
        sprintf( msg, format, "write to destination", i );
        throw QString( msg );
      }
    }
  }
  catch ( QString ex )
  {
    QgsLogger::debug( ex, 1, "qgsimageanalyzer.cpp", "QgsImageAnalyzer::extractChip()" );
    theChip = NULL;
  }

  VSIFree( buffer );
  return theChip;
}

void QgsImageAnalyzer::setTransformBand( int bandNumber )
{
  if ( bandNumber > 0 && bandNumber <= mImageData->rasterBands() )
  {
    mBandNumber = bandNumber;
  }
}


QgsImageAnalyzer::QgsGrid::QgsGrid( int bSize, QgsRasterDataset* img ):
    blocks( NULL )
{
  blockSize = bSize;
  mImageData = img;

  double dBlockSize = ( double ) blockSize;
  double xD, yD;
  xD = mImageData->imageXSize() / dBlockSize;
  yD = mImageData->imageYSize() / dBlockSize;

  cols = ceil( xD ); //47
  rows = ceil( yD ); //27
  blocks = new GridBlock[rows*cols]; //1269
  for ( int i = 0; i < ( rows*cols ); i++ )
  {
    int x = ( i % cols ) * blockSize;
    int y = ( i / cols ) * blockSize;
    GridBlock block;
    blocks[i] = block;
    blocks[i].xBegin = x;
    //if (i >= 1215 && i < 1235) printf("blocks[%d].xBegin: %d\n", i, blocks[i].xBegin);
    blocks[i].yBegin = y;
    //if (i >= 1215 && i < 1235) printf("blocks[%d].yBegin: %d\n", i, blocks[i].yBegin);
    blocks[i].xEnd = MIN( blocks[i].xBegin + ( blockSize - 1 ), mImageData->imageXSize() );
    blocks[i].yEnd = MIN( blocks[i].yBegin + ( blockSize - 1 ), mImageData->imageYSize() );

  }
}
QgsImageAnalyzer::QgsGrid::~QgsGrid()
{
  if ( blocks )
  {
    delete [] blocks;
  }
}

void QgsImageAnalyzer::QgsGrid::fillGrid( const QgsWaveletTransform::PointsList* points )
{
  int cell;
  int col, row, xPixel, yPixel;
  //double xGeo, yGeo;
  //QList<QgsGcp*> gcpList = gcpSet->list();
  typedef QgsWaveletTransform::PointsList PointsList;
  PointsList::const_iterator it = points->begin();
  for ( ; it != points->end(); ++it )
  {
//    xGeo = gcpList.at( i )->refX();
//    yGeo = gcpList.at( i )->refY();

    //mImageData->transformCoordinate( xPixel, yPixel,  xGeo, yGeo, QgsRasterDataset::TM_GEOPIXEL );

    //get cell
    QgsPoint point = ( *it );
    xPixel = ( int ) point.x();
    yPixel = ( int ) point.y();
    col = ( int ) floor( (double) xPixel / blockSize );
    row = ( int ) floor( (double) yPixel / blockSize );
    cell = row * cols + col;
    //add gcp to cell
    GridBlock& currentBlock = blocks[cell];
    currentBlock.Points.append( point );

    int leftBorder = xPixel - blockSize / 2;
    int rightBorder = xPixel + blockSize / 2;
    int topBorder = yPixel - blockSize / 2;
    int bottomBorder = yPixel + blockSize / 2;

    int additionalColumn = -1;
    int additionalRow = -1;
    //Check right and left
    if ( leftBorder < currentBlock.xBegin && col > 0 )//Should add to the block to the left
    {
      additionalColumn = col - 1;
      blocks[row * cols + additionalColumn].Points.append( point );
    }
    else if ( rightBorder > currentBlock.xEnd && col < cols - 1 ) //Should add to the block to the right
    {
      additionalColumn = col + 1;
      blocks[row * cols + col + 1].Points.append( point );
    }

    if ( topBorder < currentBlock.yBegin && row > 0 ) //Add to block above
    {
      additionalRow = row - 1;
      blocks[additionalRow * cols + col].Points.append( point );
    }
    else if ( bottomBorder > currentBlock.yEnd && row < rows - 1 ) //Add to block below
    {
      additionalRow = row + 1;
      blocks[additionalRow * cols + col].Points.append( point );
    }

    if ( additionalRow > -1 && additionalColumn > -1 )
    {
      blocks[additionalRow * cols + additionalColumn].Points.append( point );
    }
  }
}

QList<QgsPoint>* QgsImageAnalyzer::QgsGrid::gridList( QgsGcp* refGcp )
{
  int xPixel, yPixel;
  double xGeo, yGeo;
  xGeo = refGcp->refX();
  yGeo = refGcp->refY();

  mImageData->transformCoordinate( xPixel, yPixel,  xGeo, yGeo, QgsRasterDataset::TM_GEOPIXEL );
  int row = ( yPixel / blockSize );
  int col = ( xPixel / blockSize );
  if ( row < 0 || row >= rows
       || col < 0 || col >= cols )
  {//This GCP doesn't lie within the bounds of this image
    return NULL;
  }
  int cell = row * cols + col;
  //printf("cell: %d\n", cell);
  return &( blocks[cell].Points );
}

double QgsImageAnalyzer::progress()const
{
  return mProgress;
}
void QgsImageAnalyzer::setProgress( double value )
{
  mProgress = value;
}

void CPL_STDCALL QgsImageAnalyzer::setWaveletProgress( double progress, void* pProgressArg )
{
  QgsImageAnalyzer* me = static_cast<QgsImageAnalyzer*>( pProgressArg );
  me->setProgress( progress * WAVELET_PART );
}

double QgsImageAnalyzer::correlationThreshold()const
{
  return mCorrelationThreshold;
}
void QgsImageAnalyzer::setCorrelationThreshold( double value )
{
  mCorrelationThreshold = value;
}