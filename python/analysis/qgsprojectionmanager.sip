/*! \ingroup analysis*/
/*! \brief Container for Projection Information
*  Struct serves as a container for all projection related information
*  needed to set projection definitions.
*/
struct PROJ_INFO
{
%TypeHeaderCode
#include <qgsprojectionmanager.h>
%End
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

class GDALWarpOptions;

/*! \ingroup analysis
* A class for handling the projection of image chips between various coordinate systems
*/
class QgsProjectionManager
{
%TypeHeaderCode
#include <qgsprojectionmanager.h>
%End
  public:
    /*! \brief QgsProjectionManager Constructor
     * Constructs a projection manager that will handle all projection related processes.
     * \param A pointer to the GDAL reference image dataset.
     * \param A pointer to the GDAL raw image dataset.
     */
    QgsProjectionManager( QgsRasterDataset *refImageDataset, QgsRasterDataset *rawImageDataset );
    /*! \brief QgsProjectionManager Constructor
     * Constructs a projection manager that will handle all projection related processes.
     */
    QgsProjectionManager();
    /*! \brief Set the reference image dataset
     * Sets the reference image to be used for the projection as a QgsRasterDataset
     * \param A pointer to the GDAL reference image dataset.
     */
    void setReferenceImage( QgsRasterDataset *refImageDataset );
    /*! \brief Set the raw image dataset
     * Sets the raw image to be used for the projection as a QgsRasterDataset
     * \param A pointer to the GDAL raw image dataset.
     */
    void setRawImage( QgsRasterDataset *rawImageDataset );

    /*! \brief Get the reference image dataset
     * Gets the reference image to be used for the projection as a QgsRasterDataset
     */
    QgsRasterDataset* referenceImage();
    /*! \brief Get the reference image dataset
     * Gets the reference image to be used for the projection as a QgsRasterDataset
     */
    QgsRasterDataset* rawImage();
    /*! \brief QgsProjectionManager Destructor
     */
    ~QgsProjectionManager();
    /*! \brief Notifier for error messages.
     * Notifies QGSLogger of error that occured.
     * \param The message describing the error.
     */
    void notifyFailure( QString message );
    /*! \brief Project a single GCP between coordinate systems
     * A single GCP is projected between two defined coordinate systems as retrieved from
     * the raw- and reference image dataset provided with the constructor. This function handles
     * all the processes required to fully project the provided GCP chip.
     * Note: It is assumed that both datasets have valid projection definitions when called.
     * To check projection information, see QgsProjectionManager::checkProjectionInformation(...).
     * \param A pointer to the QgsRasterDataset containing the chip.
     * \param A pointer to the QgsRasterDataset in which the chip is to be returned
     */
    bool projectGCPChip( QgsRasterDataset* chipIn, QgsRasterDataset* chipOut );
    /*! \brief Project a single GCP between coordinate systems
     * A single GCP is projected between two defined coordinate systems as retrieved from
     * the raw- and reference image dataset provided with the constructor. This function handles
     * all the processes required to fully project the provided GCP chip.
     * Note: It is assumed that both datasets have valid projection definitions when called.
     * To check projection information, see QgsProjectionManager::checkProjectionInformation(...).
     * \param A pointer to the QgsGcp object containing the extracted GCP chip.
     */
    void projectGCP( QgsGcp *originalGCP );
    /*! \brief Defines the warp options for projections
     * Warp Options are defined based on information extracted from the source and destination
     * datasets.
     * \param The source GDAL dataset (original coordinate system)
     * \param The destination GDAL dataset (desired coordinate system)
     * \param True if a GDAL progress bar should be printed to standard output.
     */
    GDALWarpOptions* getWarpOptions( GDALDataset *srcDataset, GDALDataset *dstDataset, bool progressBar );
    /*! \brief Warp operation that handles projection
     * The provided dataset is warped/projected using the projection options provided as well as
     * information extracted from the GDAL raw- and reference image datasets.
     * \param The warp options as defined by QgsProjectionManager::getWarpOptions(...).
     * \param The dataset to be projected
     */
    void warpDatasets( GDALWarpOptions *pswops, GDALDataset *dstDataset );
    /*! \brief Warp data destroyer
     * Cleans up all the additional data created for the projection process
     * \param The warp options as defined by QgsProjectionManager::getWarpOptions(...).
     */
    void destroyWarpData( GDALWarpOptions *pswops );
    /*! \brief Checks for availability and validity of projection definition strings.
     * The provided dataset is checked to see if it contains the projection definition string.
     * \param The dataset to be checked.
     */
    static bool checkProjectionInformation( QgsRasterDataset *imageDataset );
    /*! \brief Sets the projection definition string
    The projection definition string is derived from information contained in the projectionInfo
    container based on the options provided, and assigned to the dataset.
    \param The dataset to which the projection definition string is to be assigned.
    \param The projection definition string.
    */
    static bool setProjectionInformation( QgsRasterDataset *imageDataset, QString projectionDefinition);
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
    static bool transformCoordinate(double* xCoords, double* yCoords,int count,const QString& destCrs,const QString& srcCrs);
    /*! \brief Tests whether the given projection strings represent the same coordinate system
     *
     * \param firstCrs The one CRS projection string
     * \param secondCrs The other CRS projection string
     * \return Returns true if the two CRS are equal, false otherwise
     *
     */
    static bool crsEquals(const QString& firstCrs,const QString& secondCrs);
};
