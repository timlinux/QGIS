/***************************************************************************
qgsprojectionmanager.cpp - The ProjectionManager class is responsible for the
projection of GCPs using the projection information either contained within the
Metadata of the image, or as specified by the user.
This class uses the GDAL Warp library to perform the projection on the provided
GCP chip.
--------------------------------------
Date : 07-May-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
Author: Francois Maass
/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsprojectionmanager.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSPROJECTIONMANAGER_H
#define QGSPROJECTIONMANAGER_H
#include <gdal_priv.h>
#include <gdalwarper.h>
#include <ogr_spatialref.h>
#include <QMap>
#include <QString>
#include "qgsgcp.h"
#include "qgsrasterdataset.h"
#include "qgslogger.h"

/*! \ingroup analysis
 *
 *  Container for Projection Information
 *  Struct serves as a container for all projection related information
 *  needed to set projection definitions.
 */
struct ANALYSIS_EXPORT PROJ_INFO
{

  QString pGeographicCSName;
  QString pProjectedCSName;
  QString pVariantName;
  QString pGeogName;
  QString pDatumName;
  QString pSpheroidName;
  QString pPMName;
  QString pAngularUnits;
  double pStdP1;
  double pStdP2;
  double pCenterLat;
  double pCenterLong;
  double pFalseEasting;
  double pFalseNorthing;
  double pStandardParallel;
  double pCentralMeridian;
  double pPseudoStdParallelLat;
  double pPseudoStdParallel1;
  double pSatelliteHeight;
  double pScale;
  double pAzimuth;
  double pRectToSkew;
  double pLat1;
  double pLong1;
  double pLat2;
  double pLong2;
  double pLatitudeOfOrigin;
  double pSemiMajor;
  double pInvFlattening;
  double pPMOffset;
  double pConvertToRadians;
  int pVariation;
  int pZone;
  int pNorth;

};


class ANALYSIS_EXPORT QgsProjectionManager
{
  public:
    /*! \brief QgsProjectionManager Constructor
    Constructs a projection manager that will handle all projection related processes.
    \param A pointer to the GDAL reference image dataset.
    \param A pointer to the GDAL raw image dataset.
    */
    QgsProjectionManager( QgsRasterDataset *refImageDataset, QgsRasterDataset *rawImageDataset );
    /*! \brief QgsProjectionManager Constructor
    Constructs a projection manager that will handle all projection related processes.
    */
    QgsProjectionManager();
    /*! \brief Set the reference image dataset
    Sets the reference image to be used for the projection as a QgsRasterDataset
    \param A pointer to the GDAL reference image dataset.
    */
    void setReferenceImage( QgsRasterDataset *refImageDataset );
    /*! \brief Set the raw image dataset
    Sets the raw image to be used for the projection as a QgsRasterDataset
    \param A pointer to the GDAL raw image dataset.
    */
    void setRawImage( QgsRasterDataset *rawImageDataset );

    /*! \brief Get the reference image dataset
    Gets the reference image to be used for the projection as a QgsRasterDataset
    */
    QgsRasterDataset* referenceImage();
    /*! \brief Get the reference image dataset
    Gets the reference image to be used for the projection as a QgsRasterDataset
    */
    QgsRasterDataset* rawImage();
    /*! \brief QgsProjectionManager Destructor
    */
    ~QgsProjectionManager();
    /*! \brief Notifier for error messages.
    Notifies QGSLogger of error that occured.
    \param The message describing the error.
    */
    void notifyFailure( QString message );

    /*! \brief Project a single GCP between coordinate systems
    A single GCP is projected between two defined coordinate systems as retrieved from
    the raw- and reference image dataset provided with the constructor. This function handles
    all the processes required to fully project the provided GCP chip.
    Note: It is assumed that both datasets have valid projection definitions when called.
    To check projection information, see QgsProjectionManager::checkProjectionInformation(...).
    \param A pointer to the QgsRasterDataset containing the chip.
    \param A pointer to the QgsRasterDataset in which the chip is to be returned
    */
    bool projectGCPChip( QgsRasterDataset* chipIn, QgsRasterDataset* chipOut );

    /*! \brief Project a single GCP between coordinate systems
    A single GCP is projected between two defined coordinate systems as retrieved from
    the raw- and reference image dataset provided with the constructor. This function handles
    all the processes required to fully project the provided GCP chip.
    Note: It is assumed that both datasets have valid projection definitions when called.
    To check projection information, see QgsProjectionManager::checkProjectionInformation(...).
    \param A pointer to the QgsGcp object containing the extracted GCP chip.
    */
    void projectGCP( QgsGcp *originalGCP );
    /*! \brief Defines the warp options for projections
    Warp Options are defined based on information extracted from the source and destination
    datasets.
    \param The source GDAL dataset (original coordinate system)
    \param The destination GDAL dataset (desired coordinate system)
    \param True if a GDAL progress bar should be printed to standard output.
    */
    GDALWarpOptions* getWarpOptions( GDALDataset *srcDataset, GDALDataset *dstDataset, bool progressBar );
    /*! \brief Warp operation that handles projection
    The provided dataset is warped/projected using the projection options provided as well as
    information extracted from the GDAL raw- and reference image datasets.
    \param The warp options as defined by QgsProjectionManager::getWarpOptions(...).
    \param The dataset to be projected
    */
    void warpDatasets( GDALWarpOptions *pswops, GDALDataset *dstDataset );
    /*! \brief Warp data destroyer
    Cleans up all the additional data created for the projection process
    \param The warp options as defined by QgsProjectionManager::getWarpOptions(...).
    */
    void destroyWarpData( GDALWarpOptions *pswops );
    /*! \brief Checks for availability and validity of projection definition strings.
    The provided dataset is checked to see if it contains the projection definition string.
    \param The dataset to be checked.
    */
    static bool checkProjectionInformation( QgsRasterDataset *imageDataset );
    /*! \brief Sets the projection definition string
    The projection definition string is derived from information contained in the projectionInfo
    container based on the options provided, and assigned to the dataset.
    \param The dataset to which the projection definition string is to be assigned.
    \param The projection definition string in PROJ4 format.
    */
    static bool setProjectionInformation( QgsRasterDataset *imageDataset, QString projectionDefinition );
    /*! \brief Gets the WKT formatted projection definition srsID
    The projection definition string is extracted from the image as a PROJ4 formatted string and converted to WKT format.
    \param The dataset from which the projection definition string is to be retrieved.
    \return The srs ID of the extracted projection definition
    */
    static long getProjectionSrsID( QgsRasterDataset *imageDataset );
    /*! \brief Gets the WKT formatted projection definition epsg
    The projection definition string is extracted from the image as a PROJ4 formatted string and converted to WKT format.
    \param The dataset from which the projection definition string is to be retrieved.
    \return The epsg of the extracted projection definition
    */
    static long getProjectionEpsg( QgsRasterDataset *imageDataset );
    /*! \brief Gets the WKT formatted projection definition authID
    The projection definition string is extracted from the image as a PROJ4 formatted string and converted to WKT format.
    \param The dataset from which the projection definition string is to be retrieved.
    \return The auth ID of the extracted projection definition
    */
    static QString getProjectionAuthID( QgsRasterDataset *imageDataset );
    /*!\brief Converts coordinates between two different coordinate reference systems
     *
     * Coordinates are converted in-place
     * \param xCoords A pointer to the first xCoordinate to transform
     * \param yCoords A pointer to the first yCoordinate to transform
     * \param count The number of coordinates to transform
     * \param destCrs The destination Coordinate system
     * \param srcCrs The source coordinate system
     * \return Returns true if successful, false otherwise
     *
     */
    static bool transformCoordinate( double* xCoords, double* yCoords, int count, const QString& destCrs, const QString& srcCrs );
    /*! \brief Tests whether the given projection strings represent the same coordinate system
     *
     * \param firstCrs The one CRS projection string
     * \param secondCrs The other CRS projection string
     * \return Returns true if the two CRS are equal, false otherwise
     *
     */
    static bool crsEquals( const QString& firstCrs, const QString& secondCrs );


  private:
    GDALDataset *mRefDataset;
    GDALDataset *mRawDataset;
    bool mFail;

};
#endif //QGSPROJECTIONMANAGER_H
