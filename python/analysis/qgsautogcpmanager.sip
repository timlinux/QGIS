/*! \ingroup analysis*/
/*! \brief Container for image information
*/
struct IMAGE_INFO
{
%TypeHeaderCode
#include <qgsautogcpmanager.h>
%End
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
struct DRIVER_INFO
{
  QString pLongName;
  QString pShortName;
  QString pIssues;
};

/*! \ingroup analysis
*/
/*! \brief Manager class for the autogcp plugin library
*/
class QgsAutoGCPManager
{
%TypeHeaderCode
#include <qgsautogcpmanager.h>
%End
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
    QgsAutoGCPManager();
    bool openSensedImage();
    bool openReferenceImage();
    IOError lastError() const;
    void setGcpCount( int count );
    void clearGcpSet();
    QgsGcpSet* extractControlPoints();
    QgsGcpSet* matchControlPoints();
    void executeOpenReferenceImage( QString path );
    void executeOpenRawImage( QString path );
    void executeExtraction();
    void executeCrossReference();
    void executeOrthorectification();
    void executeGeoreference();
    void executeGeoTransform();
    ExecutionStatus status() const;
    bool done() const;
    void setDestinationPath(QString path);
    QString destinationPath()const;
    bool createOrthoImage();
    QgsGcpSet* gcpSet();
    QgsRasterDataset* rawImage();
    QgsRasterDataset* referenceImage();
    virtual ~QgsAutoGCPManager();
    void projectGCPs();
    bool setProjection( QgsRasterDataset *imageDataset, QString projectionDefinition );
    long getProjectionSrsID( ImageFilter imageDataset );
    long getProjectionEpsg( ImageFilter imageDataset );
    QString getProjectionAuthID( ImageFilter imageDataset );
    bool exportGcpSet( QString path );
    bool importGcpSet( QString path );
    bool exportGcpSet( QgsGcpSet *gcpSet, QString path );
    void addGcp( QgsGcp *gcp );
    void updateRefGcp( QgsGcp *gcp );
    void updateRawGcp( QgsGcp *gcp );
    void removeGcp( QgsGcp *gcp );
    bool connectDatabase( QString path );
    bool connectDatabase( QString name, QString username, QString password, QString host );
    QgsGcpSet* loadDatabase(ImageFilter filter = QgsAutoGCPManager::Both);
    QgsGcpSet* loadDatabaseByLocation( double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection );
    QgsGcpSet* loadDatabaseByHash( QString hash );
    bool saveDatabase();
    bool detectGcps(ImageFilter filter = QgsAutoGCPManager::Both);
    void setChipSize( int width, int height );
    IMAGE_INFO imageInfo( QString path );
    bool isGeoreferenced() const;
    void setOutputDriver(QString driverName);
    QString outputDriver();		
    QList<DRIVER_INFO> readDriverSource(QString path);
    QString rasterFileFilter() const;
    void setExtractionBand(int nBandNumer);
    int extractionBand() const;
    double progress()const;
    void setRpcThreshold(double value);
    double rpcThreshold()const;
    double correlationThreshold()const;
    void setCorrelationThreshold( double value );
protected:
  QgsAutoGCPManager(const QgsAutoGCPManager& other);
};
