/***************************************************************************
qgsorthorectificationfilter.h - Performs orthorectification on an image using a sensor model
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

#include <qt4/QtCore/qfile.h>

/* $Id: qgsorthorectificationfilter.cpp 606 2010-10-29 09:13:39Z jamesm $ */

#include <iostream>
#include "qgsorthorectificationfilter.h"
QgsOrthorectificationFilter::QgsOrthorectificationFilter( QgsRasterDataset* image, QgsSensorModel* sensormodel, QgsSampler* sampler, QgsElevationModel* DEM )
    : QgsImageFilter( image ), mSensorModel( sensormodel ), mSampler( sampler ), mDem( DEM ), mDriver( "GTiff" )
{

}

QgsRasterDataset* QgsOrthorectificationFilter::applyFilter()
{
  if ( mSrc && mSampler && mSensorModel )
  {
    QgsLogger::debug( "Starting Orthorectification" );
    setProgress( 0.0 );
    int width = mSrc->imageXSize();
    int height = mSrc->imageYSize();
    int bands = mSrc->rasterBands();
    //GDALDataType type = mSrc->rasterDataType();
    QgsLogger::debug( QString().sprintf( "ImageSize: %d, %d - Bands: %d", width, height, bands ) );
    //Create the destination image with the same dimensions and datatype
    QString error;
    GDALDriver* driver = ( GDALDriver* ) GDALGetDriverByName( this->driver().toLatin1().data() );
    char filename[FILENAME_MAX];
    strcpy( filename, QFile::encodeName( mDestFile ).data() );
    GDALDataset* gdalDs = driver->Create( filename, width, height, bands, GDT_Byte, NULL );
    if ( !gdalDs )
    {
      return NULL;
    }

    //Copy Georeferencing and projection data
    double coeff[6];
    CPLErr result;
    if ( mSrc->geoCoefficients( coeff ) )
    {
      result = gdalDs->SetGeoTransform( coeff );
    }

    if ( mSrc->projection().size() > 0 )
    {
      char* projectionString = new char[mSrc->projection().size()+1];
      strcpy( projectionString, mSrc->projection().toLatin1().data() );
      result = gdalDs->SetProjection( projectionString );
    }

    /*****************************
     * Start Resampling process
     *****************************/
    //Create the buffer to hold the data for each line
    double totalOperations = bands * height;
    double* buffer = new double[width];
    int counter = 0;
    for ( int k = 1; k <= bands; ++k )
    {
      GDALRasterBand *band = mSrc->gdalDataset()->GetRasterBand( k );
      double min = band->GetMinimum();
      double max = band->GetMaximum();
      QgsLogger::debug( QString().sprintf( "Orthorectifying band %d", k ) );
      for ( int i = 0; i < height; ++i )
      {
        setProgress(( k*i ) / totalOperations );
        counter++;
        for ( int j = 0; j < width; ++j )
        {
          double X, Y, Z = 0.0;
          int x = j, y = i;

          transformCoordinate( coeff, x, y, X, Y );
          GDALApplyGeoTransform( coeff, ( double ) x, ( double ) y, &X, &Y );

          if ( mDem )
          {
            bool result = mDem->elevateGeographicPoint( X, Y, Z );
            if ( !result )
            {
              Z = 0.0;
            }
          }
          Z = 0.0000000001;
          double pixel = mSensorModel->c( X, Y, Z );
          double line = mSensorModel->r( X, Y, Z );
          buffer[j] = mSampler->samplePoint( mSrc, k, pixel, line );

        }

        GDALRasterBand* gdalBand = gdalDs->GetRasterBand( k );
        if ( min < 0 || max > 255 )
        {
          double *bufferValues = ( double* ) buffer;
          for ( int j = 0; j < width; ++j )
          {
            bufferValues[j] = (( bufferValues[j] - min ) / ( max - min ) ) * 255; //Scale the values to 0 -255
          }
        }
        gdalBand->RasterIO( GF_Write, 0, i, width, 1, buffer, width, 1, GDT_Float64, 0, 0 );

      }

    }
    delete [] buffer;
    GDALClose( gdalDs );

    QgsRasterDataset *ortho = new QgsRasterDataset( mDestFile );
    setProgress( 1.0 );
    return ortho;

  }
  else
  {
    if ( !mSrc )
    {
      QgsLogger::debug( "Invalid source image" );
    }
    if ( !mSampler )
    {
      QgsLogger::debug( "Invalid image sampler" );
    }
    if ( !mSensorModel )
    {
      QgsLogger::debug( "Invalid sensor model" );
    }
    return NULL;
  }

}

QString QgsOrthorectificationFilter::driver()const
{
  return mDriver;
}
void QgsOrthorectificationFilter::setDriver( QString name )
{
  mDriver = name;
}
