/***************************************************************************
qgscoordinatesystemtransform.cpp - Converts coordinates between a source and
destination coordinate system
--------------------------------------
Date : 24-October-2010
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
/* $Id: qgscoordinatesystemtransform.cpp 606 2010-10-29 09:13:39Z jamesm $ */


#include "qgscoordinatesystemtransform.h"
#include "qgslogger.h"
QgsCoordinateSystemTransform::QgsCoordinateSystemTransform():
    mSrcSet( false ),
    mDstSet( false ),
    mInitialized( false ),
    mTransformer( NULL ),
    mSrcString( "" ),
    mDstString( "" )
{

}

QgsCoordinateSystemTransform::QgsCoordinateSystemTransform( const QString& srcWkt, const QString& dstWkt ):
    mTransformer( NULL ),
    mInitialized( false )
{
  setSourceCrs( srcWkt );
  setDestinationCrs( dstWkt );
  createTransformer();
}

QgsCoordinateSystemTransform::~QgsCoordinateSystemTransform()
{
  if ( mTransformer )
  {
    delete mTransformer;
  }
}

bool QgsCoordinateSystemTransform::transform( int count, double* xCoords, double* yCoords )
{
  if ( !mInitialized )
  {
    if ( !createTransformer() )
    {
      QgsLogger::debug( "Failed to transform coordinates because the transformer could not be constructed" );
      return false;
    }
  }
  return ( mTransformer->Transform( count, xCoords, yCoords ) );

}

bool QgsCoordinateSystemTransform::transform( double srcX, double srcY, double* dstX, double* dstY )
{
  double xCoords[1] = {srcX};
  double yCoords[1] = {srcY};
  bool result = transform( 1, xCoords, yCoords );
  if ( result )
  {
    *dstX = xCoords[0];
    *dstY = yCoords[0];
    return true;
  }
  else
  {
    return false;
  }
}

void QgsCoordinateSystemTransform::setSourceCrs( const QString& srcWkt )
{
  char* crsString = new char[srcWkt.size()+1];
  //char* originalString = crsString;
  strcpy( crsString, srcWkt.toLatin1().data() );
  if ( OGRERR_NONE == mSrcCrs.importFromWkt( &crsString ) )
  {
    mSrcString = srcWkt;
    mSrcSet = true;
  }
  else
  {
    mSrcSet = false;
  }
  //delete [] originalString;
}

void QgsCoordinateSystemTransform::setDestinationCrs( const QString& dstWkt )
{
  char* crsString = new char[dstWkt.size()+1];
  strcpy( crsString, dstWkt.toLatin1().data() );
  if ( OGRERR_NONE == mDstCrs.importFromWkt( &crsString ) )
  {
    mDstString = dstWkt;
    mDstSet = true;
  }
  else
  {
    mDstSet = false;
  }
}

const QString& QgsCoordinateSystemTransform::sourceCrs() const
{
  return mSrcString;
}

const QString& QgsCoordinateSystemTransform::destinationCrs() const
{
  return mDstString;
}

bool QgsCoordinateSystemTransform::createTransformer()
{
  mTransformer = OGRCreateCoordinateTransformation( &mSrcCrs, &mDstCrs );
  if ( mTransformer )
  {
    mInitialized = true;
    return true;
  }
  else
  {
    mInitialized = false;
    return false;
  }
}

