/***************************************************************************
qgsgcp.cpp - A Ground Control Point (GCP) abstraction contains information about
 a specific GCP such as the ground and associated image coordinates of this
 GCP, as well as additional elevation and other information. This object is
 also the container for the GCP chip data.
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
/* $Id: qgsgcp.cpp 506 2010-10-16 14:02:25Z jamesm $ */
#include "qgsgcp.h"
#include <float.h>
#ifndef FLT_EQUALS
#define FLT_EQUALS(x,y) \
  (fabs(x-y)<FLT_EPSILON)
#endif

QgsGcp::QgsGcp(QString rawWkt, QString refWkt):
    mChip( NULL ),
    mXRaw( 0 ), mYRaw( 0 ),
    mXRef( 0 ), mYRef( 0 ), mZRef( 0 ),
    mCorrelationCoefficient( 0.0 )
{
  mId = 0;
  mRawWkt = rawWkt;
  mRefWkt = refWkt;
}

QgsGcp::~QgsGcp()
{

  if ( mChip )
  {
    delete mChip;
  }
}

QgsGcp::QgsGcp( const QgsGcp& other ):
    mChip( NULL ),
    mXRaw( other.mXRaw ), mYRaw( other.mYRaw ),
    mXRef( other.mXRef ), mYRef( other.mYRef ), mZRef( other.mZRef ),
    mRawWkt( other.mRawWkt ), mRefWkt( other.mRefWkt )
{

}

const QgsGcp& QgsGcp::operator=( const QgsGcp & other )
{
  mChip = NULL;
  mXRaw = other.mXRaw;
  mYRaw = other.mYRaw;
  mXRef = other.mXRef;
  mYRef = other.mYRef;
  mZRef = other.mZRef;
  return *this;
}

bool QgsGcp::operator==( const QgsGcp& other ) const
{
  return ( FLT_EQUALS( mXRef, other.mXRef ) && FLT_EQUALS( mYRef, other.mYRef ) && ( mRefWkt == other.mRefWkt ) && ( mRawWkt == other.mRawWkt ) );
}

double QgsGcp::refX() const
{
  return mXRef;
}

void QgsGcp::setRefX( double value )
{
  mXRef = value;
}

double QgsGcp::refY() const
{
  return mYRef;
}

void QgsGcp::setRefY( double value )
{
  mYRef = value;
}

double QgsGcp::refZ() const
{
  return mZRef;
}

void QgsGcp::setRefZ( double value )
{
  mZRef = value;
}

double QgsGcp::rawX() const
{
  return mXRaw;
}

void QgsGcp::setRawX( double value )
{
  mXRaw = value;
}

double QgsGcp::rawY() const
{
  return mYRaw;
}

void QgsGcp::setRawY( double value )
{
  mYRaw = value;
}

double QgsGcp::correlationCoefficient() const
{
  return mCorrelationCoefficient;
}

void QgsGcp::setCorrelationCoefficient( double value )
{
  mCorrelationCoefficient = value;
}

QgsRasterDataset* QgsGcp::gcpChip() const
{
  return mChip;
}

void QgsGcp::setGcpChip( QgsRasterDataset* value )
{
  mChip = value;
}

void QgsGcp::setRefWkt(QString wkt)
{
  mRefWkt = wkt;
}
    
QString QgsGcp::refWkt()
{
  return mRefWkt;
}
    
void QgsGcp::setRawWkt(QString wkt)
{
  mRawWkt = wkt;
}
    
QString QgsGcp::rawWkt()
{
  return mRawWkt;
}
    
QgsGcp* QgsGcp::utmGcp()
{
  if(mRawWkt == "" || mRefWkt == "")
  {
    return NULL;
  }
  
  char *wkt = NULL;
  OGRSpatialReference reference;
  reference.SetWellKnownGeogCS("EPSG:4326");
  reference.exportToWkt(&wkt);
  
  QString utmWkt(wkt);
  delete [] wkt;
  QgsGcp *gcp = new QgsGcp(utmWkt, utmWkt);
  QgsCoordinateSystemTransform transformer;
  double dstRawX, dstRawY, dstRefX, dstRefY = 0;
  
  transformer.setSourceCrs(mRawWkt);
  transformer.setDestinationCrs(utmWkt);
  if(!transformer.transform(mXRaw, mYRaw, &dstRawX, &dstRawY))
  {
    delete gcp;
    return NULL;
  }
  else
  {
    gcp->setRawX(dstRawX);
    gcp->setRawY(dstRawY);
  }
  
  transformer.setSourceCrs(mRefWkt);
  transformer.setDestinationCrs(utmWkt);
  if(!transformer.transform(mXRef, mYRef, &dstRefX, &dstRefY))
  {
    delete gcp;
    return NULL;
  }
  else
  {
    gcp->setRefX(dstRefX);
    gcp->setRefY(dstRefY);
  }
  return gcp;
}
