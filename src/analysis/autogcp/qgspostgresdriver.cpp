/***************************************************************************
qgspostgresdriver.cpp - The Driver resposible for the PostgreSQL database
connection
--------------------------------------
Date : 02-July-2010
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
/* $Id: qgspostgresdriver.cpp 598 2010-10-26 05:45:56Z goocreations $ */

#include "qgspostgresdriver.h"
#include "qgslogger.h"

QgsPostgresDriver::QgsPostgresDriver( const QString name, const QString username, const QString password, const QString host, const int port, const QgsHasher::HashAlgorithm hashAlgorithm )
{
  mName = name;
  mUsername = username;
  mPassword = password;
  mHost = host;
  mPort = port;
  setHashAlgorithm( hashAlgorithm );
  mIsOpen = false;
  openDatabase();
}

bool QgsPostgresDriver::openDatabase()
{
  if ( !mIsOpen && !mName.isNull() && !mHost.isNull() && !mUsername.isNull() && !mPassword.isNull() )
  {
    QString info = "dbname =" + mName + " user=" + mUsername + " password=" + mPassword + " host=" + mHost + " port=" + QString::number( mPort );
    mDatabase = PQconnectdb( info.toLocal8Bit() );
    if ( PQstatus( mDatabase ) != CONNECTION_OK )
    {
      mIsOpen = false;
    }
    else
    {
      mIsOpen = true;
    }
  }
  return mIsOpen;
}

bool QgsPostgresDriver::openDatabase( const QString name, const QString username, const QString password, const QString host , const int port )
{
  mName = name;
  mUsername = username;
  mPassword = password;
  mHost = host;
  mPort = port;
  if ( !mIsOpen )
  {
    QString info = "dbname =" + mName + " user=" + mUsername + " password=" + mPassword + " host=" + mHost + " port=" + QString::number( mPort );;
    mDatabase = PQconnectdb( info.toLocal8Bit() );
    if ( PQstatus( mDatabase ) != CONNECTION_OK )
    {
      mIsOpen = false;
    }
    else
    {
      mIsOpen = true;
    }
  }
  return mIsOpen;
}

PGresult* QgsPostgresDriver::executeQuery( const QString query, const QByteArray *chip )
{

  PGresult *result;
  if ( chip != NULL )
  {
    //Insert Chip here
  }
  else
  {
    //The following two lines currently replace the parameter with NULL
    QString newQuery = query;
    newQuery = newQuery.replace( "$1", "NULL" );
    result = PQexec( mDatabase, newQuery.toLocal8Bit() );
  }
  return result;
}

void QgsPostgresDriver::clearResult( PGresult* result )
{
  PQclear( result );
}

bool QgsPostgresDriver::createDatabase()
{
  try
  {
    int hashLength;
    if ( hashAlgorithm() == QgsHasher::Md5 || hashAlgorithm() == QgsHasher::Md4 )
    {
      hashLength = 32;
    }
    else if ( hashAlgorithm() == QgsHasher::Sha1 )
    {
      hashLength = 40;
    }
    if ( !openDatabase( mName, mUsername, mPassword, mHost, mPort ) )
    {
      return false;
    }
    QString queryString = "CREATE SEQUENCE imageId START 1;\nCREATE SEQUENCE gcpSetRefId START 1;\nCREATE SEQUENCE gcpSetRawId START 1;\nCREATE SEQUENCE gcpRefId START 1;\nCREATE SEQUENCE gcpRawId START 1;\n";
    clearResult( executeQuery( queryString ) );
    queryString = "CREATE TABLE image (\n";
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
    clearResult( executeQuery( queryString ) );
    queryString = "CREATE TABLE gcpSetRef (\n";
    queryString += "\tid INTEGER PRIMARY KEY NOT NULL, \n";
    queryString += "\timage INTEGER,\n";
    queryString += "\tdateCreated DATE,\n";
    queryString += "\tdateUpdated DATE,\n";
    queryString += "\tFOREIGN KEY(image) REFERENCES image(id)\n";
    queryString += ");\n";
    clearResult( executeQuery( queryString ) );
    queryString = "CREATE TABLE gcpSetRaw (\n";
    queryString += "\tid INTEGER PRIMARY KEY NOT NULL, \n";
    queryString += "\timage INTEGER,\n";
    queryString += "\tgcpSetRef INTEGER,\n";
    queryString += "\tdateCreated DATE,\n";
    queryString += "\tdateUpdated DATE,\n";
    queryString += "\tFOREIGN KEY(image) REFERENCES image(id),\n";
    queryString += "\tFOREIGN KEY(gcpSetRef) REFERENCES gcpSetRef(id)\n";
    queryString += ");\n";
    clearResult( executeQuery( queryString ) );
    queryString = "CREATE TABLE gcpRef (\n";
    queryString += "\tid INTEGER PRIMARY KEY NOT NULL, \n";
    queryString += "\tcoordinateX DECIMAL,\n";
    queryString += "\tcoordinateY DECIMAL,\n";
    queryString += "\tchip BYTEA,\n";
    queryString += "\tchipWidth INTEGER,\n";
    queryString += "\tchipHeight INTEGER,\n";
    queryString += "\tchipType character(24),\n";
    queryString += "\tchipBands INTEGER,\n";
    queryString += "\tgcpSetRef INTEGER,\n";
    queryString += "\tFOREIGN KEY(gcpSetRef) REFERENCES gcpSetRef(id)\n";
    queryString += ");\n";
    clearResult( executeQuery( queryString ) );
    queryString = "CREATE TABLE gcpRaw (\n";
    queryString += "\tid INTEGER PRIMARY KEY NOT NULL, \n";
    queryString += "\tcoordinateX DECIMAL,\n";
    queryString += "\tcoordinateY DECIMAL,\n";
    queryString += "\tgcpRef INTEGER,\n";
    queryString += "\tgcpSetRaw INTEGER,\n";
    queryString += "\tFOREIGN KEY(gcpRef) REFERENCES gcpRef(id),\n";
    queryString += "\tFOREIGN KEY(gcpSetRaw) REFERENCES gcpSetRaw(id)\n";
    queryString += ");\n";
    clearResult( executeQuery( queryString ) );
    return true;
  }
  catch ( QtConcurrent::Exception e )
  {
    return false;
  }
}

int QgsPostgresDriver::insertImage( int width, int height, QString hash, double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection )
{
  QString queryString = "SELECT id FROM image WHERE hash = '" + hash + "';";
  PGresult *result = executeQuery( queryString );
  int imageId = -1;
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
  if ( PQresultStatus( result ) == PGRES_TUPLES_OK && PQntuples( result ) < 1 ) // if image doesn't exists
  {
    clearResult( result );
    queryString = "INSERT INTO image (id, width, height, hash, pixelWidthX, pixelWidthY, pixelHeightX, pixelHeightY, topLeftX, topLeftY, projection, dateCreated, dateUpdated) VALUES (";
    queryString += "nextval('imageId'), ";
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
    queryString += "CURRENT_TIMESTAMP, ";
    queryString += "CURRENT_TIMESTAMP);";
    clearResult( executeQuery( queryString ) );
    queryString = "SELECT currval('imageId') AS imageId FROM image;";
    result = executeQuery( queryString );
    imageId = QString( PQgetvalue( result, 0, PQfnumber( result, "imageId" ) ) ).toInt();
    clearResult( result );
  }
  else
  {
    imageId = QString( PQgetvalue( result, 0, PQfnumber( result, "id" ) ) ).toInt();
    clearResult( result );
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
    queryString += "dateUpdated = CURRENT_TIMESTAMP ";
    queryString += "WHERE id = " + QString::number( imageId ) + ";";
    clearResult( executeQuery( queryString ) );
  }
  return imageId;
}

int QgsPostgresDriver::insertReferenceSet( int id, int image )
{
  QString queryString = "SELECT id FROM gcpSetRef WHERE id = " + QString::number( id ) + ";";
  PGresult *result = executeQuery( queryString );
  int setId = -1;
  if ( PQresultStatus( result ) == PGRES_TUPLES_OK && PQntuples( result ) < 1 ) // if set doesn't exists
  {
    clearResult( result );
    queryString = "SELECT id ";
    queryString = "INSERT INTO gcpSetRef (id, image, dateCreated, dateUpdated) VALUES (";
    queryString += "nextval('gcpSetRefId'), ";
    queryString += QString::number( image ) + ", ";
    queryString += "CURRENT_TIMESTAMP, ";
    queryString += "CURRENT_TIMESTAMP);";
    clearResult( executeQuery( queryString ) );
    queryString = "SELECT currval('gcpSetRefId') AS setId FROM gcpSetRef;";
    result = executeQuery( queryString );
    setId = QString( PQgetvalue( result, 0, PQfnumber( result, "setId" ) ) ).toInt();
    clearResult( result );
  }
  else
  {
    setId = QString( PQgetvalue( result, 0, PQfnumber( result, "id" ) ) ).toInt();
    clearResult( result );
    queryString = "UPDATE gcpSetRef SET\n";
    queryString += "image = " + QString::number( image ) + ", ";
    queryString += "dateUpdated = CURRENT_TIMESTAMP ";
    queryString += "WHERE id = " + QString::number( setId ) + ";";
    clearResult( executeQuery( queryString ) );
  }
  return setId;
}

int QgsPostgresDriver::insertRawSet( int id, int image, int referenceSet )
{
  QString queryString = "SELECT id FROM gcpSetRaw WHERE id = " + QString::number( id ) + ";";
  PGresult *result = executeQuery( queryString );
  int setId = -1;
  if ( PQresultStatus( result ) == PGRES_TUPLES_OK && PQntuples( result ) < 1 ) // if set doesn't exists
  {
    clearResult( result );
    queryString = "INSERT INTO gcpSetRaw (id, image, gcpSetRef, dateCreated, dateUpdated) VALUES (";
    queryString += "nextval('gcpSetRawId'), ";
    queryString += QString::number( image ) + ", ";
    queryString += QString::number( referenceSet ) + ", ";
    queryString += "CURRENT_TIMESTAMP, ";
    queryString += "CURRENT_TIMESTAMP);";
    clearResult( executeQuery( queryString ) );
    queryString = "SELECT currval('gcpSetRawId') AS setId FROM gcpSetRaw;";
    result = executeQuery( queryString );
    setId = QString( PQgetvalue( result, 0, PQfnumber( result, "setId" ) ) ).toInt();
    clearResult( result );
  }
  else
  {
    setId = QString( PQgetvalue( result, 0, PQfnumber( result, "id" ) ) ).toInt();
    clearResult( result );
    queryString = "UPDATE gcpSetRaw SET\n";
    queryString += "image = " + QString::number( image ) + ", ";
    queryString += "gcpSetRef = " + QString::number( referenceSet ) + ", ";
    queryString += "dateUpdated = CURRENT_TIMESTAMP ";
    queryString += "WHERE id = " + QString::number( setId ) + ";";
    clearResult( executeQuery( queryString ) );
  }
  return setId;
}

int QgsPostgresDriver::insertReferenceGcp( int id, double coordinateX, double coordinateY, QByteArray *chip, int chipWidth, int chipHeight, QString chipType, int chipBands, int referenceSet )
{
  QString queryString = "SELECT id FROM gcpRef WHERE id = " + QString::number( id ) + ";";
  PGresult *result = executeQuery( queryString );
  int gcpId = -1;
  QString var1;
  var1.sprintf( DOUBLE_FORMAT, coordinateX );
  QString var2;
  var2.sprintf( DOUBLE_FORMAT, coordinateY );
  if ( PQresultStatus( result ) == PGRES_TUPLES_OK && PQntuples( result ) < 1 ) // if gcp doesn't exists
  {
    clearResult( result );
    queryString = "INSERT INTO gcpRef (id, coordinateX, coordinateY, chip, chipWidth, chipHeight, chipType, chipBands, gcpSetRef) VALUES (";
    queryString += "nextval('gcpRefId'), ";
    queryString += var1 + ", ";
    queryString += var2 + ", ";
    queryString += "$1, ";
    queryString += QString::number( chipWidth ) + ", ";
    queryString += QString::number( chipHeight ) + ", ";
    queryString += "'" + chipType + "', ";
    queryString += QString::number( chipBands ) + ", ";
    queryString += QString::number( referenceSet ) + ");";
    executeQuery( queryString, chip );
    queryString = "SELECT currval('gcpRefId') AS gcpId FROM gcpRef;";
    result = executeQuery( queryString );
    gcpId = QString( PQgetvalue( result, 0, PQfnumber( result, "gcpId" ) ) ).toInt();
    clearResult( result );
  }
  else
  {
    gcpId = QString( PQgetvalue( result, 0, PQfnumber( result, "id" ) ) ).toInt();
    clearResult( result );
    queryString = "UPDATE gcpRef SET\n";
    queryString += "coordinateX = " + var1 + ", ";
    queryString += "coordinateY = " + var2 + ", ";
    queryString += "chip = $1, ";
    queryString += "chipWidth = " + QString::number( chipWidth ) + ", ";
    queryString += "chipHeight = " + QString::number( chipHeight ) + ", ";
    queryString += "chipType = '" + chipType + "', ";
    queryString += "chipBands = " + QString::number( chipBands ) + ", ";
    queryString += "gcpSetRef = " + QString::number( referenceSet ) + " ";
    queryString += "WHERE id = " + QString::number( gcpId ) + ";";
    executeQuery( queryString, chip );
  }
  return gcpId;
}

int QgsPostgresDriver::insertRawGcp( int id, double coordinateX, double coordinateY, int referenceGcp, int rawSet )
{
  QString queryString = "SELECT id FROM gcpRaw WHERE id = " + QString::number( id ) + ";";
  PGresult *result = executeQuery( queryString );
  int gcpId = -1;
  QString var1;
  var1.sprintf( DOUBLE_FORMAT, coordinateX );
  QString var2;
  var2.sprintf( DOUBLE_FORMAT, coordinateY );
  if ( PQresultStatus( result ) == PGRES_TUPLES_OK && PQntuples( result ) < 1 ) // if gcp doesn't exists
  {
    clearResult( result );
    queryString = "INSERT INTO gcpRaw (id, coordinateX, coordinateY, gcpRef, gcpSetRaw) VALUES (";
    queryString += "nextval('gcpRawId'), ";
    queryString += var1 + ", ";
    queryString += var2 + ", ";
    queryString += QString::number( referenceGcp ) + ", ";
    queryString += QString::number( rawSet ) + ");";
    clearResult( executeQuery( queryString ) );
    queryString = "SELECT currval('gcpRawId') AS gcpId FROM gcpRaw;";
    result = executeQuery( queryString );
    gcpId = QString( PQgetvalue( result, 0, PQfnumber( result, "gcpId" ) ) ).toInt();
    clearResult( result );
  }
  else
  {
    gcpId = QString( PQgetvalue( result, 0, PQfnumber( result, "id" ) ) ).toInt();
    clearResult( result );
    queryString = "UPDATE gcpRaw SET\n";
    queryString += "coordinateX = " + var1 + ", ";
    queryString += "coordinateY = " + var2 + ", ";
    queryString += "gcpSetRaw = " + QString::number( rawSet ) + " ";
    queryString += "WHERE id = " + QString::number( gcpId ) + ";";
    clearResult( executeQuery( queryString ) );
  }
  return gcpId;
}

QgsGcpSet* QgsPostgresDriver::selectByHash( const QString refHash, const QString rawHash )
{
  QgsGcpSet *set = new QgsGcpSet();
  QString queryString = "SELECT gcpSetRef.id AS id FROM gcpSetRef, gcpSetRaw, image WHERE gcpSetRef.image = image.id AND image.hash = '" + refHash + "' AND gcpSetRaw.gcpSetRef = gcpSetRef.id AND gcpSetRaw.image = image.id AND image.hash = '" + rawHash + "';";
  PGresult *result = executeQuery( queryString );
  int refId = QString( PQgetvalue( result, 0, PQfnumber( result, "id" ) ) ).toInt();
  if ( refId < 1 )
  {
    queryString = "SELECT gcpSetRef.id AS id FROM gcpSetRef, image WHERE gcpSetRef.image = image.id AND image.hash = '" + refHash + "';";
    result = executeQuery( queryString );
    refId = QString( PQgetvalue( result, 0, PQfnumber( result, "id" ) ) ).toInt();
  }
  set->setRefId( refId );
  if ( refId > 0 )
  {
    clearResult( result );
    queryString = "SELECT gcpRef.id AS gcpId, gcpSetRef.id AS setId, coordinateX, coordinateY, chip, chipWidth, chipHeight, chipType, chipBands FROM gcpRef, gcpSetRef WHERE gcpSetRef.id = " + QString::number( refId ) + " AND gcpRef.gcpSetRef = gcpSetRef.id;";
    result = executeQuery( queryString );
  }

  int counter = 0;
  int resultSize = PQntuples( result );
  int idIndex = PQfnumber( result, "gcpId" );
  int xIndex = PQfnumber( result, "coordinateX" );
  int yIndex = PQfnumber( result, "coordinateY" );
  int chipIndex = PQfnumber( result, "chip" );
  int widthIndex = PQfnumber( result, "chipWidth" );
  int heightIndex = PQfnumber( result, "chipHeight" );
  int typeIndex = PQfnumber( result, "chipType" );
  int bandsIndex = PQfnumber( result, "chipBands" );
  while ( counter < resultSize )
  {
    QgsGcp *gcp = new QgsGcp();
    gcp->setId( QString( PQgetvalue( result, counter, idIndex ) ).toInt() );
    gcp->setRefX( QString( PQgetvalue( result, counter, xIndex ) ).toDouble() );
    gcp->setRefY( QString( PQgetvalue( result, counter, yIndex ) ).toDouble() );
    //char *data = PQgetvalue(result, counter, chipIndex);
    //QgsImageChip *chip = QgsImageChip::createImage( QString(PQgetvalue(result, counter, widthIndex )).toInt(), QString(PQgetvalue(result, counter, heightIndex )).toInt(),  GDALGetDataTypeByName( QString(PQgetvalue(result, counter, typeIndex )).trimmed().toLatin1().data() ), QString(PQgetvalue(result, counter, bandsIndex )).toInt(), data );
    //gcp->setGcpChip( chip );
    queryString = "SELECT gcpRaw.id AS gcpId, gcpSetRaw.id AS setId, gcpRaw.coordinateX AS coordinateX, gcpRaw.coordinateY AS coordinateY FROM gcpRaw, gcpSetRef, gcpSetRaw, gcpRef WHERE gcpRaw.gcpRef = " + QString( PQgetvalue( result, counter, idIndex ) ) + " AND gcpRaw.gcpSetRaw = gcpSetRaw.id AND gcpSetRaw.gcpSetRef = " + QString::number( refId ) + ";";
    PGresult *result2 = executeQuery( queryString );
    int setIdIndex = PQfnumber( result2, "setId" );
    int rawIdIndex = PQfnumber( result2, "gcpId" );
    if ( QString( PQgetvalue( result2, counter, rawIdIndex ) ).toInt() > 0 )
    {
      set->setRawId( QString( PQgetvalue( result2, counter, setIdIndex ) ).toInt() );
      int rawXIndex = PQfnumber( result2, "coordinateX" );
      int rawYIndex = PQfnumber( result2, "coordinateY" );
      gcp->setRawX( QString( PQgetvalue( result2, counter, rawXIndex ) ).toDouble() );
      gcp->setRawY( QString( PQgetvalue( result2, counter, rawYIndex ) ).toDouble() );
      clearResult( result2 );
    }
    set->addGcp( gcp );
    counter++;
  }
  clearResult( result );
  return set;
}

QgsGcpSet* QgsPostgresDriver::selectByLocation( double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection )
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
  QString queryString = "SELECT gcpSetRef.id AS id FROM gcpSetRef, image WHERE gcpSetRef.image = image.id AND image.pixelWidthX = " + var1 + " AND image.pixelWidthY = " + var2 + " AND image.pixelHeightX = " + var3 + " AND image.pixelHeightY = " + var4 + " AND image.topLeftX = " + var5 + " AND image.topLeftY = " + var6 + " AND image.projection = '" + projection + "';";
  PGresult *result = executeQuery( queryString );
  int refId = QString( PQgetvalue( result, 0, PQfnumber( result, "id" ) ) ).toInt();
  set->setRefId( refId );
  if ( refId > 0 )
  {
    clearResult( result );
    queryString = "SELECT gcpRef.id AS gcpId, gcpSetRef.id AS setId, coordinateX, coordinateY, chip, chipWidth, chipHeight, chipType, chipBands FROM gcpRef, gcpSetRef WHERE gcpSetRef.id = " + QString::number( refId ) + " AND gcpRef.gcpSetRef = gcpSetRef.id;";
    result = executeQuery( queryString );
  }

  int counter = 0;
  int resultSize = PQntuples( result );

  int idIndex = PQfnumber( result, "gcpId" );
  int xIndex = PQfnumber( result, "coordinateX" );
  int yIndex = PQfnumber( result, "coordinateY" );
  int chipIndex = PQfnumber( result, "chip" );
  int widthIndex = PQfnumber( result, "chipWidth" );
  int heightIndex = PQfnumber( result, "chipHeight" );
  int typeIndex = PQfnumber( result, "chipType" );
  int bandsIndex = PQfnumber( result, "chipBands" );
  while ( counter < resultSize )
  {
    QgsGcp *gcp = new QgsGcp();
    gcp->setId( QString( PQgetvalue( result, counter, idIndex ) ).toInt() );
    gcp->setRefX( QString( PQgetvalue( result, counter, xIndex ) ).toDouble() );
    gcp->setRefY( QString( PQgetvalue( result, counter, yIndex ) ).toDouble() );
    //char *data = PQgetvalue(result, 0, chipIndex);
    //QgsImageChip *chip = QgsImageChip::createImage( QString(PQgetvalue(result, counter, widthIndex )).toInt(), QString(PQgetvalue(result, counter, heightIndex )).toInt(),  GDALGetDataTypeByName( QString(PQgetvalue(result, counter, typeIndex )).trimmed().toLatin1().data() ), QString(PQgetvalue(result, counter, bandsIndex )).toInt(), data );
    //gcp->setGcpChip( chip );
    set->addGcp( gcp );
    clearResult( result );
    counter++;
  }
  return set;
}

