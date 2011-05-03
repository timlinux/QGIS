/***************************************************************************
qgsimagechip.cpp - Extension of the QgsRasterDataset that enables in-memory storage
 of the image data
--------------------------------------
Date : 12-September-2010
Author : James Meyer
Copyright : (C) 2010 by C.J. Meyer
Email : jamesmeyerx@gmail.com

/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsimagechip.cpp 606 2010-10-29 09:13:39Z jamesm $ */

#include "qgsimagechip.h"
#include "qgslogger.h"

QgsImageChip::QgsImageChip( GDALDataset* dataset ):
    QgsRasterDataset( dataset ), mData( NULL )
{
}
QgsImageChip::QgsImageChip( const QgsImageChip& other ): QgsRasterDataset( NULL ), mData( NULL )
{

}

const QgsImageChip&  QgsImageChip::operator =( const QgsImageChip & other )
{
	return other;
}

QgsImageChip* QgsImageChip::createImageChip( int width, int height, GDALDataType type,  int bands )
{
  char message[200];
  registerGdalDrivers();
  long chipDataSize = width * height * bands * ( GDALGetDataTypeSize( type ) / 8 );

  //printf("Allocate CHip data %d\n", chipDataSize);
  //printf("%d, %d, %d, %d\n", width, height, bands, (GDALGetDataTypeSize(type) / 8));
  void* chipData = VSIMalloc( chipDataSize );
  //printf("Allocated CHip data\n");
  if ( !chipData )
  {
    sprintf( message, "Failed to allocate memory (%ld Bytes) for GCP Chip data", chipDataSize );
    QgsLogger::debug( message, 1, "qgsimagechip.cpp", "QgsImageChip::createImageChip()" );
    return NULL;
  }
  char nameString[200];
  sprintf( nameString,
           "MEM:::DATAPOINTER=%p,PIXELS=%d,LINES=%d,BANDS=%d,DATATYPE=%s",
           chipData, width, height, bands, GDALGetDataTypeName( type ) );
  GDALDataset* ds = ( GDALDataset* )GDALOpen( nameString, GA_Update );
  if ( ! ds )
  {
    QgsLogger::debug( "Cannot create In-Memory Chip dataset", 1, "qgsimagechip.cpp", "QgsImageChip::createImageChip()" );
    return NULL;
  }

  QgsImageChip* theChip = new QgsImageChip( ds );
  theChip->mDataSize = chipDataSize;
  theChip->mData = chipData;

  return theChip;
}

QgsImageChip* QgsImageChip::createImage( int width, int height, GDALDataType type, int bands, void* data )
{
  registerGdalDrivers();
  long chipDataSize = width * height * bands * ( GDALGetDataTypeSize( type ) / 8 );

  //printf("Allocated CHip data\n");
  if ( !data )
  {
    QgsLogger::debug( "Invalid memory pointer for GCP Chip data" );
    return NULL;
  }
  char nameString[200];
  sprintf( nameString,
           "MEM:::DATAPOINTER=%p,PIXELS=%d,LINES=%d,BANDS=%d,DATATYPE=%s",
           data, width, height, bands, GDALGetDataTypeName( type ) );
  GDALDataset* ds = ( GDALDataset* )GDALOpen( nameString, GA_Update );
  if ( ! ds )
  {
    QgsLogger::debug( "Cannot create In-Memory Chip dataset" );
    return NULL;
  }

  QgsImageChip* theChip = new QgsImageChip( ds );
  theChip->mDataSize = chipDataSize;
  theChip->mData = data;
  return theChip;
}

QgsImageChip::~QgsImageChip()
{
  if ( mDataset )
  {
    GDALClose(( GDALDatasetH )mDataset );
    mDataset = NULL;
  }
  if ( mData )
  {
    VSIFree( mData );
  }
}

