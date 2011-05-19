/***************************************************************************
qgssqlitedriver.cpp - The Driver resposible for the SQLite database
connection
--------------------------------------
Date : 02-July-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgssqlitedriver.cpp 606 2010-10-29 09:13:39Z jamesm $ */

#include "qgssqlitedriver.h"
#include "qgslogger.h"

QgsSqliteDriver::QgsSqliteDriver( const QString path, const QgsHasher::HashAlgorithm hashAlgorithm )
{
  mPath = path;
  setHashAlgorithm( hashAlgorithm );
  mIsOpen = false;
  openDatabase();
}

bool QgsSqliteDriver::openDatabase()
{
  if ( !mIsOpen && !mPath.isNull() )
  {
    int rc = sqlite3_open( mPath.toAscii().data(), &mDatabase );
    if ( rc )
    {
      mIsOpen = false;
      sqlite3_close( mDatabase );
    }
    else
    {
      mIsOpen = true;
    }
  }
  return mIsOpen;
}

bool QgsSqliteDriver::openDatabase( const QString path )
{
  mPath = path;
  return openDatabase();
}

void QgsSqliteDriver::deleteDatabase()
{
  closeDatabase();
  QFile::remove( mPath );
}

QUERY_RESULT QgsSqliteDriver::executeQuery( const QString query, const QByteArray *chip )
{
  QUERY_RESULT result;
  char **res;
  int row, col;
  char *error;
  if ( chip != NULL )
  {
    //result.bindValue( QString( ":chip" ), QByteArray( *chip ) ); //copy constructor is needed to ensure that the data is correclty referenced
  }

  int rc = sqlite3_get_table( mDatabase, query.toAscii().data(), &res, &row, &col, &error );
  if ( rc != SQLITE_OK )
  {
    //QgsLogger::debug( "A error occured during the SQL execution.\nQuery: " + query + "\nError: " + QString( error ) );
  }
  else
  {
    for ( int i = 0; i < col; ++i )
    {
      result.pHeader.append( QString( res[i] ) );
    }
    for ( int i = 0; i < col*row; ++i )
    {
      result.pData.append( QString( res[i+col] ) );
    }
    sqlite3_free_table( res );
  }
  return result;
}

bool QgsSqliteDriver::createDatabase()
{
  try
  {
    int hashLength = 32;
    if ( hashAlgorithm() == QgsHasher::Md5 || hashAlgorithm() == QgsHasher::Md4 )
    {
      hashLength = 32;
    }
    else if ( hashAlgorithm() == QgsHasher::Sha1 )
    {
      hashLength = 40;
    }
    if ( !openDatabase( mPath ) )
    {
      return false;
    }
    QString queryString = "CREATE TABLE image (\n";
    queryString += "\tid INTEGER PRIMARY KEY NOT NULL, \n";
    queryString += "\twidth DECIMAL,\n";
    queryString += "\theight DECIMAL,\n";
    queryString += "\thash char(" + QString::number( hashLength ) + "),\n";
    queryString += "\tpixelWidthX DECIMAL,\n";
    queryString += "\tpixelWidthY DECIMAL,\n";
    queryString += "\tpixelHeightX DECIMAL,\n";
    queryString += "\tpixelHeightY DECIMAL,\n";
    queryString += "\ttopLeftX DECIMAL,\n";
    queryString += "\ttopLeftY DECIMAL,\n";
    queryString += "\tprojection TEXT,\n";
    queryString += "\tdateCreated DATE,\n";
    queryString += "\tdateUpdated DATE\n";
    queryString += ");\n";
    executeQuery( queryString );
    queryString = "CREATE TABLE gcpSetRef (\n";
    queryString += "\tid INTEGER PRIMARY KEY NOT NULL, \n";
    queryString += "\timage INTEGER,\n";
    queryString += "\tdateCreated DATE,\n";
    queryString += "\tdateUpdated DATE,\n";
    queryString += "\tFOREIGN KEY(image) REFERENCES image(id)\n";
    queryString += ");\n";
    executeQuery( queryString );
    queryString = "CREATE TABLE gcpSetRaw (\n";
    queryString += "\tid INTEGER PRIMARY KEY NOT NULL, \n";
    queryString += "\timage INTEGER,\n";
    queryString += "\tgcpSetRef INTEGER,\n";
    queryString += "\tdateCreated DATE,\n";
    queryString += "\tdateUpdated DATE,\n";
    queryString += "\tFOREIGN KEY(image) REFERENCES image(id),\n";
    queryString += "\tFOREIGN KEY(gcpSetRef) REFERENCES gcpSetRef(id)\n";
    queryString += ");\n";
    executeQuery( queryString );
    queryString = "CREATE TABLE gcpRef (\n";
    queryString += "\tid INTEGER PRIMARY KEY NOT NULL, \n";
    queryString += "\tcoordinateX DECIMAL,\n";
    queryString += "\tcoordinateY DECIMAL,\n";
    queryString += "\tchip BLOB,\n";
    queryString += "\tchipWidth INTEGER,\n";
    queryString += "\tchipHeight INTEGER,\n";
    queryString += "\tchipType char(24),\n";
    queryString += "\tchipBands INTEGER,\n";
    queryString += "\tgcpSetRef INTEGER,\n";
    queryString += "\tFOREIGN KEY(gcpSetRef) REFERENCES gcpSetRef(id)\n";
    queryString += ");\n";
    executeQuery( queryString );
    queryString = "CREATE TABLE gcpRaw (\n";
    queryString += "\tid INTEGER PRIMARY KEY NOT NULL, \n";
    queryString += "\tcoordinateX DECIMAL,\n";
    queryString += "\tcoordinateY DECIMAL,\n";
    queryString += "\tgcpRef INTEGER,\n";
    queryString += "\tgcpSetRaw INTEGER,\n";
    queryString += "\tFOREIGN KEY(gcpRef) REFERENCES gcpRef(id),\n";
    queryString += "\tFOREIGN KEY(gcpSetRaw) REFERENCES gcpSetRaw(id)\n";
    queryString += ");\n";
    executeQuery( queryString );
    return true;
  }
  catch ( QtConcurrent::Exception e )
  {
    return false;
  }
}

int QgsSqliteDriver::insertImage( int width, int height, QString hash, double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection )
{
  QString queryString = "SELECT id FROM image WHERE hash = '" + hash + "';";
  QUERY_RESULT result = executeQuery( queryString );
  QString imageId = "-1";
  QString var1;
  var1.sprintf( DOUBLE_FORMAT, pixelWidthX );
  QString var2;
  var2.sprintf( DOUBLE_FORMAT, pixelWidthY );
  QString var3;
  var3.sprintf( DOUBLE_FORMAT, pixelHeightX );
  QString var4;
  var4.sprintf( DOUBLE_FORMAT, pixelHeightY );
  QString var5;
  var5.sprintf( DOUBLE_FORMAT, topLeftX );
  QString var6;
  var6.sprintf( DOUBLE_FORMAT, topLeftY );
  if ( queryValue( result, 0 ).toInt() < 1 ) // if image doesn't exists
  {
    queryString = "INSERT INTO image (width, height, hash, pixelWidthX, pixelWidthY, pixelHeightX, pixelHeightY, topLeftX, topLeftY, projection, dateCreated, dateUpdated) VALUES (";
    queryString += QString::number( width ) + ", ";
    queryString += QString::number( height ) + ", ";
    queryString += "'" + hash + "', ";
    queryString += var1 + ", ";
    queryString += var2 + ", ";
    queryString += var3 + ", ";
    queryString += var4 + ", ";
    queryString += var5 + ", ";
    queryString += var6 + ", ";
    queryString += "'" + projection + "', ";
    queryString += "datetime('now'), ";
    queryString += "datetime('now'));";
    executeQuery( queryString );
    queryString = "SELECT last_insert_rowid() AS imageId FROM image;";
    result = executeQuery( queryString );
    imageId = queryValue( result, 0 );
  }
  else
  {
    imageId = queryValue( result, 0 );
    queryString = "UPDATE image SET\n";
    queryString += "width = " + QString::number( width ) + ", ";
    queryString += "height = " + QString::number( height ) + ", ";
    queryString += "hash = '" + hash + "', ";
    queryString += "pixelWidthX = " + var1 + ", ";
    queryString += "pixelWidthY = " + var2 + ", ";
    queryString += "pixelHeightX = " + var3 + ", ";
    queryString += "pixelHeightY = " + var4 + ", ";
    queryString += "topLeftX = " + var5 + ", ";
    queryString += "topLeftY = " + var6 + ", ";
    queryString += "projection = '" + projection + "', ";
    queryString += "dateUpdated = datetime('now') ";
    queryString += "WHERE id = " + imageId + ";";
    executeQuery( queryString );
  }
  return imageId.toInt();
}

int QgsSqliteDriver::insertReferenceSet( int id, int image )
{
  QString queryString = "SELECT id FROM gcpSetRef WHERE id = " + QString::number( id ) + ";";
  QUERY_RESULT result = executeQuery( queryString );
  QString setId = "-1";
  if ( queryValue( result, 0 ).toInt() < 1 ) // if set doesn't exists
  {
    QString queryString = "SELECT id ";
    queryString = "INSERT INTO gcpSetRef (image, dateCreated, dateUpdated) VALUES (";
    queryString += QString::number( image ) + ", ";
    queryString += "datetime('now'), ";
    queryString += "datetime('now'));";
    executeQuery( queryString );
    queryString = "SELECT last_insert_rowid() AS setId FROM gcpSetRef;";
    result = executeQuery( queryString );
    setId = queryValue( result, 0 );
  }
  else
  {
    setId = queryValue( result, 0 );
    queryString = "UPDATE gcpSetRef SET\n";
    queryString += "image = " + QString::number( image ) + ", ";
    queryString += "dateUpdated = datetime('now') ";
    queryString += "WHERE id = " + setId + ";";
    executeQuery( queryString );
  }
  return setId.toInt();
}

int QgsSqliteDriver::insertRawSet( int id, int image, int referenceSet )
{
  QString queryString = "SELECT id FROM gcpSetRaw WHERE id = " + QString::number( id ) + ";";
  QUERY_RESULT result = executeQuery( queryString );
  QString setId = "-1";
  if ( queryValue( result, 0 ).toInt() < 1 ) // if set doesn't exists
  {
    queryString = "INSERT INTO gcpSetRaw (image, gcpSetRef, dateCreated, dateUpdated) VALUES (";
    queryString += QString::number( image ) + ", ";
    queryString += QString::number( referenceSet ) + ", ";
    queryString += "datetime('now'), ";
    queryString += "datetime('now'));";
    executeQuery( queryString );
    queryString = "SELECT last_insert_rowid() AS setId FROM gcpSetRaw;";
    result = executeQuery( queryString );
    setId = queryValue( result, 0 );
  }
  else
  {
    setId = queryValue( result, 0 );
    queryString = "UPDATE gcpSetRaw SET\n";
    queryString += "image = " + QString::number( image ) + ", ";
    queryString += "gcpSetRef = " + QString::number( referenceSet ) + ", ";
    queryString += "dateUpdated = datetime('now') ";
    queryString += "WHERE id = " + setId + ";";
    executeQuery( queryString );
  }
  return setId.toInt();
}

int QgsSqliteDriver::insertReferenceGcp( int id, double coordinateX, double coordinateY, QByteArray *chip, int chipWidth, int chipHeight, QString chipType, int chipBands, int referenceSet )
{
  QString queryString = "SELECT id FROM gcpRef WHERE id = " + QString::number( id ) + ";";
  QUERY_RESULT result = executeQuery( queryString );
  QString var1;
  var1.sprintf( DOUBLE_FORMAT, coordinateX );
  QString var2;
  var2.sprintf( DOUBLE_FORMAT, coordinateY );
  QString gcpId = "-1";
  if ( queryValue( result, 0 ).toInt() < 1 ) // if set doesn't exists
  {
    queryString = "INSERT INTO gcpRef (coordinateX, coordinateY, chip, chipWidth, chipHeight, chipType, chipBands, gcpSetRef) VALUES (";
    queryString += var1 + ", ";
    queryString += var2 + ", ";
    queryString += ":chip, ";
    queryString += QString::number( chipWidth ) + ", ";
    queryString += QString::number( chipHeight ) + ", ";
    queryString += "'" + chipType + "', ";
    queryString += QString::number( chipBands ) + ", ";
    queryString += QString::number( referenceSet ) + ");";
    //executeQuery( queryString, chip );
    executeQuery( queryString );
    queryString = "SELECT last_insert_rowid() AS gcpId FROM gcpRef;";
    result = executeQuery( queryString );
    gcpId = queryValue( result, 0 );
  }
  else
  {
    gcpId = queryValue( result, 0 );
    queryString = "UPDATE gcpRef SET\n";
    queryString += "coordinateX = " + var1 + ", ";
    queryString += "coordinateY = " + var2 + ", ";
    queryString += "chip = :chip, ";
    queryString += "chipWidth = " + QString::number( chipWidth ) + ", ";
    queryString += "chipHeight = " + QString::number( chipHeight ) + ", ";
    queryString += "chipType = '" + chipType + "', ";
    queryString += "chipBands = " + QString::number( chipBands ) + ", ";
    queryString += "gcpSetRef = " + QString::number( referenceSet ) + " ";
    queryString += "WHERE id = " + gcpId + ";";
    //executeQuery( queryString, chip );
    executeQuery( queryString );
  }
  return gcpId.toInt();
}

int QgsSqliteDriver::insertRawGcp( int id, double coordinateX, double coordinateY, int referenceGcp, int rawSet )
{
  QString queryString = "SELECT id FROM gcpRaw WHERE id = " + QString::number( id ) + ";";
  QUERY_RESULT result = executeQuery( queryString );
  QString var1;
  var1.sprintf( DOUBLE_FORMAT, coordinateX );
  QString var2;
  var2.sprintf( DOUBLE_FORMAT, coordinateY );
  QString gcpId = "-1";
  if ( queryValue( result, 0 ).toInt() < 1 ) // if set doesn't exists
  {
    queryString = "INSERT INTO gcpRaw (coordinateX, coordinateY, gcpRef, gcpSetRaw) VALUES (";
    queryString += var1 + ", ";
    queryString += var2 + ", ";
    queryString += QString::number( referenceGcp ) + ", ";
    queryString += QString::number( rawSet ) + ");";
    executeQuery( queryString );
    queryString = "SELECT last_insert_rowid() AS gcpId FROM gcpRaw;";
    result = executeQuery( queryString );
    gcpId = queryValue( result, 0 );
  }
  else
  {
    gcpId = queryValue( result, 0 );
    queryString = "UPDATE gcpRaw SET\n";
    queryString += "coordinateX = " + var1 + ", ";
    queryString += "coordinateY = " + var2 + ", ";
    queryString += "gcpSetRaw = " + QString::number( rawSet ) + " ";
    queryString += "WHERE id = " + gcpId + ";";
    executeQuery( queryString );
  }
  return gcpId.toInt();
}

QgsGcpSet* QgsSqliteDriver::selectByHash( const QString refHash, const QString rawHash )
{
  QgsGcpSet *set = new QgsGcpSet();
  QString queryString = "SELECT gcpSetRef.id FROM gcpSetRef, gcpSetRaw, image WHERE gcpSetRef.image = image.id AND image.hash = '" + refHash + "' AND gcpSetRaw.gcpSetRef = gcpSetRef.id AND gcpSetRaw.image = image.id AND image.hash = '" + rawHash + "';";
  QUERY_RESULT result = executeQuery( queryString );
  QString refId = queryValue( result, 0 );
  if ( refId.toInt() < 1 )
  {
    queryString = "SELECT gcpSetRef.id FROM gcpSetRef, image WHERE gcpSetRef.image = image.id AND image.hash = '" + refHash + "';";
    result = executeQuery( queryString );
    refId = queryValue( result, 0 );
  }
  set->setRefId( refId.toInt() );
  if ( refId.length() > 0 )
  {
    queryString = "SELECT gcpRef.id AS gcpId, gcpSetRef.id AS setId, coordinateX, coordinateY, chip, chipWidth, chipHeight, chipType, chipBands FROM gcpRef, gcpSetRef WHERE gcpSetRef.id = " + refId + " AND gcpRef.gcpSetRef = gcpSetRef.id;";
    result = executeQuery( queryString );
    while ( result.pData.size() > 0 )
    {
      QgsGcp *gcp = new QgsGcp();
      gcp->setId( queryValue( result, "gcpId" ).toInt() );
      gcp->setRefX( queryValue( result, "coordinateX" ).toDouble() );
      gcp->setRefY( queryValue( result, "coordinateY" ).toDouble() );
      //QByteArray *data = new QByteArray( queryValue( result, "chip" ).toByteArray() );
      //QgsImageChip *chip = QgsImageChip::createImage( queryValue( result, "chipWidth" ).toInt(), queryValue( result, "chipHeight" ).toInt(),  GDALGetDataTypeByName( queryValue( result, "chipType" ).trimmed().toLatin1().data() ), queryValue( result, "chipBands" ).toInt(), data->data() );
      //gcp->setGcpChip( chip );;
      QString gcpId = queryValue( result, "gcpId" );
      queryString = "SELECT gcpRaw.id AS gcpId, gcpSetRaw.id AS setId, gcpRaw.coordinateX AS coordinateX, gcpRaw.coordinateY AS coordinateY FROM gcpRaw, gcpSetRef, gcpSetRaw, gcpRef WHERE gcpRaw.gcpRef = " + gcpId + " AND gcpRaw.gcpSetRaw = gcpSetRaw.id AND gcpSetRaw.gcpSetRef = " + refId + ";";
      QUERY_RESULT result2 = executeQuery( queryString );
      if ( queryValue( result2, "gcpId" ).toInt() > 0 )
      {
        set->setRawId( queryValue( result2, "setId" ).toInt() );
        gcp->setRawX( queryValue( result2, "coordinateX" ).toDouble() );
        gcp->setRawY( queryValue( result2, "coordinateY" ).toDouble() );

      }
      for ( int i = 0; i < 9; i++ ) //Remove all the values returned from one row: 9 values were retrieved per row, hence 9 have to be removed
      {
        result.pData.pop_front();
      }
      set->addGcp( gcp );
    }
  }
  return set;
}

QgsGcpSet* QgsSqliteDriver::selectByLocation( double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection )
{
  QString var1;
  var1.sprintf( DOUBLE_FORMAT, pixelWidthX );
  QString var2;
  var2.sprintf( DOUBLE_FORMAT, pixelWidthY );
  QString var3;
  var3.sprintf( DOUBLE_FORMAT, pixelHeightX );
  QString var4;
  var4.sprintf( DOUBLE_FORMAT, pixelHeightY );
  QString var5;
  var5.sprintf( DOUBLE_FORMAT, topLeftX );
  QString var6;
  var6.sprintf( DOUBLE_FORMAT, topLeftY );
  QgsGcpSet *set = new QgsGcpSet();
  QString queryString = "SELECT gcpSetRef.id FROM gcpSetRef, image WHERE gcpSetRef.image = image.id AND image.pixelWidthX = " + var1 + " AND image.pixelWidthY = " + var2 + " AND image.pixelHeightX = " + var3 + " AND image.pixelHeightY = " + var4 + " AND image.topLeftX = " + var5 + " AND image.topLeftY = " + var6 + " AND image.projection = '" + projection + "';";
  QUERY_RESULT result = executeQuery( queryString );
  QString refId = queryValue( result, 0 );
  set->setRefId( refId.toInt() );
  if ( refId.length() > 0 )
  {
    queryString = "SELECT gcpRef.id AS gcpId, gcpSetRef.id AS setId, coordinateX, coordinateY, chip, chipWidth, chipHeight, chipType, chipBands FROM gcpRef, gcpSetRef WHERE gcpSetRef.id = " + refId + " AND gcpRef.gcpSetRef = gcpSetRef.id;";
    result = executeQuery( queryString );
    while ( result.pData.size() > 0 )
    {
      QgsGcp *gcp = new QgsGcp();
      gcp->setId( queryValue( result, "gcpId" ).toInt() );
      gcp->setRefX( queryValue( result, "coordinateX" ).toDouble() );
      gcp->setRefY( queryValue( result, "coordinateY" ).toDouble() );
      //QByteArray *data = new QByteArray( queryValue( result, "chip" ).toByteArray() );
      //QgsImageChip *chip = QgsImageChip::createImage( queryValue( result, "chipWidth" ).toInt(), queryValue( result, "chipHeight" ).toInt(),  GDALGetDataTypeByName( queryValue( result, "chipType" ).trimmed().toLatin1().data() ), queryValue( result, "chipBands" ).toInt(), data->data() );
      //gcp->setGcpChip( chip );
      set->addGcp( gcp );
      for ( int i = 0; i < 9; i++ ) //Remove all the values returned from one row: 9 values were retrieved per row, hence 9 have to be removed
      {
        result.pData.pop_front();
      }
    }
  }
  return set;
}

void QgsSqliteDriver::closeDatabase()
{
  if ( mIsOpen )
  {
    sqlite3_close( mDatabase );
    mIsOpen = false;
  }
}

bool QgsSqliteDriver::reconnect()
{
  closeDatabase();
  return openDatabase();
}

QString QgsSqliteDriver::lastError()
{
  return QString( sqlite3_errmsg( mDatabase ) );
}

QString QgsSqliteDriver::queryValue( QUERY_RESULT results, int position )
{
  QString res = "-1";
  if ( results.pData.size() > 0 )
  {
    res = results.pData.at( position );
  }
  return res;
}

QString QgsSqliteDriver::queryValue( QUERY_RESULT results, QString header )
{
  QString res = "-1";
  if ( results.pData.size() > 0 )
  {
    int position = results.pHeader.indexOf( header );
    res = results.pData.at( position );
  }
  return res;
}
