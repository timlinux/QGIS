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
/* $Id: qgsdatasource.h 506 2010-10-16 14:02:25Z jamesm $ */
#ifndef QGSDATASOURCE_H
#define QGSDATASOURCE_H
#define HASH_SIZE 1000000
#include "qgshasher.h"
#include "qgsgcpset.h"
#include "qgsrasterdataset.h"
#include "qgssqlitedriver.h"
//#include "qgspostgresdriver.h"
#include "qgsimagechip.h"
#include <QString>
#include <QList>
#include <QDate>
#include <QByteArray>
#include <QtCore>
/*! \ingroup analysis*/
/*! \brief The DataSource class is the data access layer between the
* program logic and the database. For different installations of the system,
* the DataSource may use a different Connector to interface with different
* database engines. The DataSource performs operations such as constructing
* mapping objects from database records, as well as the inverse of making these
* objects persistent.
*/
class ANALYSIS_EXPORT QgsDataSource
{
  public:
    /*!\brief Enumerator to define the type of database
     * [1] SQLite database
     * [2] Postgres database
    */
    enum DatabaseType
    {
      SQLite,
      Postgres
    };
    /*!\brief Constructor for a QgsDataSource
    * \param type The type of database to be used, based on the enumerator values
    * \param hashAlgorithm The hashing algorithm to be used for the database, defaults to MD5
    */
    QgsDataSource( const DatabaseType type, const QgsHasher::HashAlgorithm hashAlgorithm = QgsHasher::Md5 );
    /*!\brief Create a SQLite database
    * \param path The path where the database file is to be stored
    * \return True if database creation was successful
    */
    bool createDatabase( const QString path );
    /*!\brief Create a PostgreSQL database
    * \param name The name of the database
    * \param username The username for the database
    * \param password The password for the database
    * \param host The address/location of the database
    * \return True if database creation was successful
    */
    bool createDatabase( const QString name, const QString username, const QString password, const QString host );
    /*!\brief Insert an entire GCP Set into the database
      * \param gcpSet The QgsGcpSet containing all the GCPs to be stored in the database
      * \param refImage The reference image that corresponds to the GCPs. Only metadata from the image is stored in the database
      * \param rawImage The raw image that corresponds to the GCPs. Only metadata from the image is stored in the database
      * \return True on successful insert
      */
    bool insertGcpSet( const QgsGcpSet *gcpSet, const QgsRasterDataset *refImage, const QgsRasterDataset *rawImage );
    /*!\brief Select an entire GCP Set based on a specific image
      * \param refImage The reference image to which the GCPs belong
      * \param rawImage The raw image to which the GCPs belong
      * \return A QgsGcpSet populated with all the GCPs belonging to the specified images
      */
    QgsGcpSet* selectByImage( const QgsRasterDataset *refImage, const QgsRasterDataset *rawImage );
    /*!\brief Select an entire GCP Set based on the hash values of the specified images
      * \param refHash The hash value of the reference image to which the GCPs belong
      * \param rawHash The hash value of the raw image to which the GCPs belong
      * \return A QgsGcpSet populated with all the GCPs belonging to the specified images
      */
    QgsGcpSet* selectByHash( const QString refHash, const QString rawHash );
    /*!\brief Select an entire GCP Set based on the location of the images
      * \param pixelWidthX x component of the pixel width
      * \param pixelWidthY y component of the pixel widht
      * \param pixelHeightX x component of the pixel height
      * \param pixelHeightY y component of the pixel height
      * \param topLeftX x coordinate of the center of the upper left pixel
      * \param topLeftY y coordinate of the center of the upper left pixel
      * \param projection The projection definition
      * \return A QgsGcpSet populated with all the GCPs matching the specified criteria
      */
    QgsGcpSet* selectByLocation( double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection );
    /*!\brief Returns the database SQL-Driver
    */
    QgsSqlDriver* driver();
    /*!\brief Deletes the database SQL-Driver
    */
    //void deleteDriver();
    /*!\brief Returns the Hash Algorithm used in the database
    */
    QgsHasher::HashAlgorithm hashAlgorithm();
    /*!\brief Set the hash algorithm to be used in the database
    * \param hashAlgorithm The hashing algorithm to be used for the database
    */
    void setHashAlogorithm( QgsHasher::HashAlgorithm hashAlgorithm );
    /*!\brief Select an entire GCP Set based on the location of the GCPs
    * \param latitude The latitude of the required GCPs
    * \param longitude The longitude of the required GCPs
    * \param window The window in which the GCPs are located
    * \return A QgsGcpSet populated with all the GCPs matching the specified location
    */
    QgsGcpSet selectByLocation( double latitude, double longitude, double window );
    /*!\brief Store a GCP Dataset
    * \param -- The dataset containing the GCPs to be stored
    */
    //bool storeGCPSet( QgsGcpSet );
    /*!\brief Returns the database type
    */
    DatabaseType type() {return mType;};
    /*!\brief Destructor for QgsDataSource
    */
    virtual ~QgsDataSource();

  private:
    bool createSqliteDatabase( const QString path );
    bool createPostgresDatabase( const QString name, const QString username, const QString password, const QString host );

    QgsHasher::HashAlgorithm mHashAlgorithm;
    DatabaseType mType;
    QgsHasher *mHasher;
    QgsSqliteDriver *mSqliteDriver;
    //QgsPostgresDriver *mPostgresDriver;
};

#endif // QGSDATASOURCE_H
