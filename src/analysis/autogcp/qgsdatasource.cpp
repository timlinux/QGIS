/***************************************************************************
qgsdatasource.cpp - The DataSource class is the data access layer between the
 program logic and the database. For different installations of the system,
 the DataSource may use a different Connector to interface with different
 database engines. The DataSource performs operations such as constructing
 mapping objects from database records, as well as the inverse of making these
 objects persistent.
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
/* $Id: qgsdatasource.cpp 506 2010-10-16 14:02:25Z jamesm $ */

#include "qgsdatasource.h"
#include <QMessageBox>

#include "qgslogger.h"

QgsDataSource::QgsDataSource( const DatabaseType type, const QgsHasher::HashAlgorithm hashAlgorithm )
{
  mHashAlgorithm = hashAlgorithm;
  mHasher = new QgsHasher();
  mType = type;
}

bool QgsDataSource::createDatabase( const QString path )
{
  if ( mType == SQLite )
  {
    return createSqliteDatabase( path );
  }
  return false;
}

bool QgsDataSource::createDatabase( const QString name, const QString username, const QString password, const QString host )
{
  if ( mType == Postgres )
  {
    //return createPostgresDatabase( name, username, password, host );
  }
  return false;
}

bool QgsDataSource::createSqliteDatabase( const QString path )
{
  mSqliteDriver = new QgsSqliteDriver( path, mHashAlgorithm );
  return mSqliteDriver->createDatabase();
}

bool QgsDataSource::createPostgresDatabase( const QString name, const QString username, const QString password, const QString host )
{
  /*mPostgresDriver = new QgsPostgresDriver( name, username, password, host, mHashAlgorithm );
  return mPostgresDriver->createDatabase();*/
  return false;
}

bool QgsDataSource::insertGcpSet( const QgsGcpSet *gcpSet, const QgsRasterDataset *refImage, const QgsRasterDataset *rawImage )
{
  try
  {

    QgsSqlDriver *databaseDriver = NULL;
    if ( mType == SQLite )
    {
      databaseDriver = mSqliteDriver;
    }
    else if ( mType == Postgres )
    {
      /*databaseDriver = new QgsPostgresDriver();
      databaseDriver->openDatabase();
      databaseDriver->createDatabase();*/
    }
    if ( databaseDriver != NULL )
    {
      QString refHash = mHasher->getFileHash( refImage->filePath(), mHashAlgorithm, HASH_SIZE );
      int refImageId = databaseDriver->insertImage( refImage->imageXSize(), refImage->imageYSize(), refHash, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, "projectionString" );
      if ( refImageId < 1 )
      {
        return false;
      }
      QList<QgsGcp*> set = gcpSet->constList();
      bool rawGcpExists = false;
      for ( int i = 0; i < set.size(); i++ )
      {
        QgsGcp *gcp = set[i];
        if ( gcp->rawX() != 0.0 || gcp->rawY() != 0.0 )
        {
          rawGcpExists = true;
          break;
        }
      }

      int refSetId = gcpSet->refId();
      if ( refImageId > 0 )
      {
        refSetId = databaseDriver->insertReferenceSet( refSetId, refImageId );
      }
      int rawSetId = gcpSet->rawId();
      if ( rawGcpExists && rawImage && refSetId > 0 )
      {
        QString rawHash = mHasher->getFileHash( rawImage->filePath(), mHashAlgorithm, HASH_SIZE );
        int rawImageId = databaseDriver->insertImage( rawImage->imageXSize(), rawImage->imageYSize(), rawHash, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, "projectionString" );
        if ( rawImageId > 0 )
        {
          rawSetId = databaseDriver->insertRawSet( rawSetId, rawImageId, refSetId );
        }
      }

      for ( int i = 0; i < set.size(); i++ )
      {
        if ( refSetId > 0 )
        {
          QgsGcp *gcp = set[i];
          QgsImageChip *chip = NULL;
          QByteArray *data = NULL;
          if ( mType == SQLite )
          {
            chip = dynamic_cast<QgsImageChip*>( gcp->gcpChip() );
            data = new QByteArray((( char* ) chip->data() ), chip->dataSize() );
          }
          else if ( mType == Postgres )
          {
            chip = NULL;
            data = NULL;
          }
          int refGcpId = databaseDriver->insertReferenceGcp( gcp->id(), gcp->refX(), gcp->refY(), data, gcp->gcpChip()->imageXSize(), gcp->gcpChip()->imageYSize(), QString( GDALGetDataTypeName( gcp->gcpChip()->rasterDataType() ) ), gcp->gcpChip()->rasterBands(), refSetId );
          if ( rawSetId > 0 && refGcpId > 0 )
          {
            databaseDriver->insertRawGcp( gcp->id(), gcp->rawX(), gcp->rawY(), refGcpId, rawSetId );
          }
        }
      }
      return true;
    }
    return false;
  }
  catch ( QtConcurrent::Exception e )
  {
    return false;
  }
}

QgsGcpSet* QgsDataSource::selectByImage( const QgsRasterDataset *refImage, const QgsRasterDataset *rawImage )
{
  QString refHash = mHasher->getFileHash( refImage->filePath(), mHashAlgorithm, HASH_SIZE );
  QString rawHash = "";
  if ( rawImage )
  {
    rawHash = mHasher->getFileHash( rawImage->filePath(), mHashAlgorithm, HASH_SIZE );
  }
  return selectByHash( refHash, rawHash );
}

QgsGcpSet* QgsDataSource::selectByHash( const QString refHash, const QString rawHash )
{
  /*mPostgresDriver = new QgsPostgresDriver();
  mPostgresDriver->openDatabase();
  mPostgresDriver->createDatabase();*/
  QgsGcpSet *set = NULL;
  if ( mType == SQLite )
  {
    if ( refHash.length() > 0 )
    {
      set = mSqliteDriver->selectByHash( refHash, rawHash );
    }
  }
  else if ( mType == Postgres )
  {
    if ( refHash.length() > 0 )
    {
      //set = mPostgresDriver->selectByHash( refHash, rawHash );
    }
  }
  return set;
}

QgsGcpSet* QgsDataSource::selectByLocation( double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection )
{
  QgsGcpSet *set = NULL;
  if ( mType == SQLite )
  {
    set = mSqliteDriver->selectByLocation( pixelWidthX, pixelWidthY, pixelHeightX, pixelHeightY, topLeftX, topLeftY, projection );
  }
  else if ( mType == Postgres )
  {
    //set = mPostgresDriver->selectByLocation( pixelWidthX, pixelWidthY, pixelHeightX, pixelHeightY, topLeftX, topLeftY, projection );
  }
  return set;
}

QgsSqlDriver* QgsDataSource::driver()
{
  if ( mType == SQLite )
  {
    return mSqliteDriver;
  }
  else if ( mType == Postgres )
  {
    //return mPostgresDriver;
    return NULL;
  }
  return NULL;
}

/*void QgsDataSource::deleteDriver()
{
  if ( mType == SQLite )
  {
    mSqliteDriver->deleteDatabase();
  }
  else if ( mType == Postgres )
  {
    mPostgresDriver->deleteDatabase();
  }
}*/

QgsHasher::HashAlgorithm QgsDataSource::hashAlgorithm()
{
  return mHashAlgorithm;
}

void QgsDataSource::setHashAlogorithm( QgsHasher::HashAlgorithm hashAlgorithm )
{
  mHashAlgorithm = hashAlgorithm;
}

QgsGcpSet QgsDataSource::selectByLocation( double latitude, double longitude, double window )
{
  QgsGcpSet gcpDS;
  return gcpDS;
}

QgsDataSource::~QgsDataSource()
{
}
