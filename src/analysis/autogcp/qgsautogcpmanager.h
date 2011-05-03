/***************************************************************************
qgsmanager.cpp - This class manages the operations that need to be performed by
 the plug-in interface. Due to the clear separation of the core analysis
 classes and the plugin-specific user interface classes, a bridge is required
 between these different libraries. This class acts as the bridge, and removes
 any business logic from the presentational duties of the UI classes. This is
 done by delegating more specific operations to dedicated classes in the core
 package.
--------------------------------------
Date : 07-May-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
Modified: Francois Maass
Modification Date: 24/07/2010
/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsautogcpmanager.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSAUTOGCPMANAGER_H
#define QGSAUTOGCPMANAGER_H
#include "qgsgcpset.h"
#include "qgsrasterdataset.h"
#include "qgsrasterdataset.h"
#include "qgsprojectionmanager.h"
#include "qgsdatasource.h"
#include "qgsorthorectificationfilter.h"
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#ifndef DEFAULT_CHIP_SIZE
#define DEFAULT_CHIP_SIZE 32
#endif

/*! \ingroup analysis*/
/*! \brief Container for image information
*/
struct ANALYSIS_EXPORT IMAGE_INFO
{
  QString pFileName;
  QString pFilePath;
  QString pHash;
  QString pFileFormat;
  QString pFileSize;
  QString pRasterWidth;
  QString pRasterHeight;
  QString pOriginCoordinates;
  QString pPixelSize;
  QDateTime pDateCreated;
  QDateTime pDateModified;
  QDateTime pDateRead;
  QString pRasterBands;
  QString pProjectionInfo;
};

/*! \ingroup analysis*/
/*! \brief Container for the output driver information
*/
struct ANALYSIS_EXPORT DRIVER_INFO
{
  QString pLongName;
  QString pShortName;
  QString pIssues;
};

/*! \ingroup analysis
*/
/*! \brief Manager class for the autogcp plugin
 *
 * This class serves as the outer interface to the automated GCP detection and orthorectification classes.
 * It is specifically designed to easily manage the entire process from extraction to orthorectification.
*/
class ANALYSIS_EXPORT QgsAutoGCPManager
{
  public:
    /*!\brief Enumerator to define the type of database filter
     * [1] Reference image only
     * [2] Raw image only
     * [3] Reference and raw image
    */
    enum ImageFilter
    {
      Reference = 0,
      Raw = 1,
      Both = 2
    };
    enum IOError
    {
      ErrorNone = 0,
      ErrorFileNotFound = 1,
      ErrorOpenFailed = 2,
      ErrorMissingProjection = 3
    };
    enum ExecutionStatus
    {
      StatusDone = 0,
      StatusFailed = 1,
      StatusBusy = 2
    };
    enum TransformFunction
    {
      ThinPlateSpline = 0
    };
    QgsAutoGCPManager();

    /*! \brief  Opens the sensed (raw) image to use
    */
    bool openSensedImage();
    /*! \brief  Opens the reference (control) image to use
    */
    bool openReferenceImage();
    IOError lastError() const;
    /*! \brief Sets the destination filename where the ortho-image will be saved.
     *
     */
    void setDestinationPath( QString path );
    /*! \brief Gets the destination filename where the ortho-image will be saved.
     *
     */
    QString destinationPath()const;
    /*! \brief  Sets the amount of GCP's to extract from the reference image.
    */
    void setGcpCount( int count );
    /*! \brief Empty the current set of GCPs
    */
    void clearGcpSet();
    /*! \brief  Performs the extraction of Ground Control Points from the reference image.
    This class is the owner of the returned dataset and destroys it when itself is destroyed.
    \return A pointer to the managed GCP set or NULL if the operation failed.
    */
    QgsGcpSet* extractControlPoints();

    /*! \brief  Matches the current set of owned GCP's to the raw image
    This class is the owner of the returned dataset and destroys it when itself is destroyed.
    \return A pointer to the managed GCP set, or NULL if the operation failed.
    */
    QgsGcpSet* matchControlPoints();
    /*! \brief Opens the reference image in another thread*/
    void executeOpenReferenceImage( QString path );
    /*! \brief Opens the raw image in another thread*/
    void executeOpenRawImage( QString path );
    /*! \brief  Performs the extraction of Ground Control Points from the reference image in another thread.
     *
     * This method returns immediately, while processing continues in another thread.
     * Use status() to check for completion of the operation.
     * Once completed, call gcpList() to get the resulting list.
     *
     */
    void executeExtraction();

    /*! \brief Performs the cross-referencing of  GCPs in another thread
     *
     * \sa executeExtraction();
     */
    void executeCrossReference();

    /*! \brief Performs orthorectification in another thread*/
    void executeOrthorectification();
    /*! \brief Performs GCP geotransform in another thread*/
    void executeGeoTransform();
    /*! \brief Performs image georeferencing in another thread
     *
     */
    void executeGeoreference();

    /*! \brief Gets the current status of execution
     *
     * \return The ExecutionStatus of this manager.
     *
     */
    ExecutionStatus status() const {return mExecStatus;}
    /*! \brief Sets the current status of the QgsAutoGCPManager */
    void setStatus( ExecutionStatus eStat ) {mExecStatus = eStat;}
    /*! \brief Checks whether the manager has completed all operations, if any.
     *
     * This function will also return true if the previous operation failed.
     *
     * \return Returns true if no operation is currently executing.
     */
    bool done() const;

    /*! \brief  Gets a pointer to the managed GCP set
    This class is the owner of the returned dataset and destroys it when itself is destroyed.
    \return A pointer to the managed GCP set.
    */
    bool createOrthoImage();
    /*! \brief Returns a pointer to the QgsGcpSet maintained by this manager*/
    QgsGcpSet* gcpSet();
    /*! \brief  Returns the raw image.
    Return a pointer to the QgsRasterDataset containing the raw image dataset.
    */
    QgsRasterDataset* rawImage() {return mRawImage;}
    /*! \brief  Returns the reference image.
    Return a pointer to the QgsRasterDataset containing the reference image dataset.
    */
    QgsRasterDataset* referenceImage() {return mRefImage;}
    /*! \brief  QgsAutoGCPManager destructor.
    Deallocates all datasets created from any operations.
    */
    virtual ~QgsAutoGCPManager();
    /*! \brief Projects all the GCPs stored in an instance of the class
     */
    void projectGCPs();
    /*! \brief Set the projection of a GDAL image dataset based on the information in the PROJ_INFO struct
     * \param imageDataset The GDAL dataset to which the projection definition should be assigned
     * \param projectionInfo The PROJ4 projection definition string to be set
     */
    bool setProjection( QgsRasterDataset *imageDataset, QString projectionDefinition );
    /*! \brief Gets the WKT formatted projection definition srsID
     * The projection definition string is extracted from the image as a PROJ4 formatted string and converted to WKT format.
     * \param imageDataset The enum value for the required dataset
     * \return The srs ID of the extracted projection definition
     */
    long getProjectionSrsID( ImageFilter imageDataset );
    /*! \brief Gets the WKT formatted projection definition epsg
     * The projection definition string is extracted from the image as a PROJ4 formatted string and converted to WKT format.
     * \param imageDataset The enum value for the required dataset
     * \return The epsg of the extracted projection definition
     */
    long getProjectionEpsg( ImageFilter imageDataset );
    /*! \brief Gets the WKT formatted projection definition authID
     * The projection definition string is extracted from the image as a PROJ4 formatted string and converted to WKT format.
     * \param imageDataset The enum value for the required dataset
     * \return The auth ID of the extracted projection definition
     */
    QString getProjectionAuthID( ImageFilter imageDataset );
    /*! \brief Exports all the GCPs previously stored to a points file
     * \param path The path where the exported file should be stored
     * \return True on successful export
     */
    bool exportGcpSet( QString path );
    /*! \brief Imports the GCPs previously stored in a points file
     * \param path The path where the import file is located
     * \return True on successful import
     */
    bool importGcpSet( QString path );
    /*! \brief Exports all the GCPs stored in the provided GCP set
     * \param gcpSet A pointer to the QgsGcpSet that contains the GCPs to be exported
     * \param path The path where the exported file should be stored
     * \return True on successful export
     */
    bool exportGcpSet( QgsGcpSet *gcpSet, QString path );
    /*! \brief Add a gcp to the GCP set
     * \param gcp A pointer to the GCP that is to be added
     */
    void addGcp( QgsGcp *gcp );
    /*! \brief Update a GCP that was previously stored
     * \param gcp A pointer to the GCP that needs to be updated
     */
    void updateRefGcp( QgsGcp *gcp );
    /*! \brief Update a GCP that was previously stored
     * \param gcp A pointer to the GCP that needs to be updated
     */
    void updateRawGcp( QgsGcp *gcp );
    /*! \brief Remove a previously stored GCP
     * \param gcp A pointer to the GCP that is to be removed
     */
    void removeGcp( QgsGcp *gcp );
    /*! \brief Open the specified database and retrieve the GCP set that belongs to the currently stored raw and reference images
     * \param path The path to the Sqlite database file
     * \return True on successful connection
     */
    bool connectDatabase( QString path );
    /*! \brief Open the specified database and retrieve the GCP set that belongs to the currently stored raw and reference images
     * \param name The name of the database
     * \param username The username for the database
     * \param password The password for the database
     * \param host The address/location of the database
     * \return True on successful connection
     */
    bool connectDatabase( QString name, QString username, QString password, QString host );
    /*! \brief Open the specified database file as an active database and retrieve the GCP set that belongs to the currently stored raw and reference images
     * \param filter The image selection filter
     * \return A QgsGcpSet containing all the GCPs that belongs to the currently stored raw and reference images
     */
    QgsGcpSet* loadDatabase( ImageFilter filter = QgsAutoGCPManager::Both );
    /*! \brief Open the specified database file as an active database and retrieve the GCP set that fall within the provided location boundaries
     * \param pixelWidthX x component of the pixel width
     * \param pixelWidthY y component of the pixel widht
     * \param pixelHeightX x component of the pixel height
     * \param pixelHeightY y component of the pixel height
     * \param topLeftX x coordinate of the center of the upper left pixel
     * \param topLeftY y coordinate of the center of the upper left pixel
     * \param projection The projection definition
     * \return A QgsGcpSet containing all the GCPs that fall within the provided location boundaries
     */
    QgsGcpSet* loadDatabaseByLocation( double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection );
    /*! \brief Open the specified database and retrieve the GCP set that belongs to the provided hash value
     * \param hash The hash value specifying which GCPs are required
     * \return A QgsGcpSet containing all the GCPs that belongs to the provided hash value
     */
    QgsGcpSet* loadDatabaseByHash( QString hash );
    /*! \brief Commit all the stored GCPs to the database.  The current QgsGcpSet will be committed to the database provided
     */
    bool saveDatabase();
    /*! \brief Checks if there are GCPs stored in the database for the given reference image
     * \param filter The image selection filter
     * \return True if the GCPs are detected in the database
     */
    bool detectGcps( ImageFilter filter = QgsAutoGCPManager::Both );
    /*! \brief Set the size of the GCP chips that will be extracted
     * \param width The width of the chip
     * \param height The height of the chip
     */
    void setChipSize( int width, int height );
    /*! \brief Returns the image information in the form of the IMAGE_INFO struct
     * \param path The path of the image from which the information is to be collected
     * \return The IMAGE_INFO struct containing all the image information
     */
    IMAGE_INFO imageInfo( QString path );
    /*! \brief Checks whether the current set of GCPs have already been cross-referenced with the raw image.*/
    bool isGeoreferenced() const;
    /*! \brief Sets the output driver to use creating rectified rasters*/
    void setOutputDriver( QString driverName ) {mOutputDriver = driverName;}
    /*! \brief Gets the driver currently being used to create output rasters*/
    QString outputDriver() {return mOutputDriver;}
    /*! \brief Reads and returns all the GDAL drivers that support the creation of raster images
     * \param path The path of the driver SQLite database file
     * \return The DRIVER_INFO list of all supported output drivers for raster images
     */
    QList<DRIVER_INFO> readDriverSource( QString path );
    /*! \brief Gets the file filter string to use in calls to QFileDialog*/
    QString rasterFileFilter() const {return mRasterFileFilter;}
    /*! \brief Sets the band number that must be used to perform extraction and cross-referencing
     *
     * If an invalid band number is given, the closest band inside the valid range is used.
     */
    void setExtractionBand( int nBandNumer );
    /*! \brief Gets the band number currently being used to perform extractions and cross-referencing
     */
    int extractionBand() const {return mExtractionBand;}
    /*! \brief Gets the progress of the currently executing operation
     *
     * \return The current operation's progress, or 0 if no operation is executing
     */
    double progress();
    /*! \brief Sets the GDAL transform function that should be used
     *
     * @note Currently only ThinPlateSpline is supported.
     */
    void setTransformFunction( TransformFunction eTransformFunc ) {mTransformFunc = eTransformFunc;}
    /*! \brief Sets the RMS Error threshold for construction of the RPC model*/
    void setRpcThreshold( double value ) {mRpcThreshold = value;}
    /*! \brief Gets the RMS Error threshold begin used for the RPC model*/
    double rpcThreshold()const {return mRpcThreshold;}
    /*! \brief Gets the currently used correlation threshold*/
    double correlationThreshold()const;
    /*! \brief Sets the correlation threshold*/
    void setCorrelationThreshold( double value );

    void setGeoReferenced( bool value );
  protected:
    QgsAutoGCPManager( const QgsAutoGCPManager& other );
  private: //TYPES
    typedef double( QgsAutoGCPManager::*ProgressFunc )( void* progressArgs )const;
    typedef bool ( QgsAutoGCPManager::*ExecutionFunc )();
    class ExecutionThread : public QThread
    {
      public:
        ExecutionThread( ExecutionFunc operation, QgsAutoGCPManager* pManager );
      protected:
        void run();
      private:
        QgsAutoGCPManager* mManager;
        ExecutionFunc mOperation;
    };

    //Operation Functions
    bool extract();
    bool crossReference();
    bool georeference();
    bool geoTransform();

    void executeOperation( ExecutionFunc operation, QgsRasterDataset* pDs );

  private:
    QString refPath;
    QString rawPath;
    ExecutionStatus mExecStatus;
    ExecutionThread* mExecThread;
    QgsRasterDataset* mRawImage;
    QgsRasterDataset* mRefImage;
    QgsRasterDataset* mOrtho;
    IOError mLastError;
    QgsGcpSet* mGcpSet;
    QgsProjectionManager* mProjManager;
    QString mDestPath;
    int mNumGcps;
    int mChipWidth;
    int mChipHeight;
    int mExtractionBand;
    bool mGeoReferenced;
    QgsDataSource *mDataSource;
    QString mOutputDriver;
    QString mRasterFileFilter;
    TransformFunction mTransformFunc;
    ProgressFunc mProgressFunc;
    QgsAbstractOperation* mProgressArg;
    QMutex mMutex;
    double mRpcThreshold;
    double mCorrelationThreshold;




};

#endif // QGSMANAGER_H
