/*! \ingroup analysis*/
/*! \brief The DataSource class is the data access layer between the
* program logic and the database. For different installations of the system,
* the DataSource may use a different Connector to interface with different
* database engines. The DataSource performs operations such as constructing
* mapping objects from database records, as well as the inverse of making these
* objects persistent.
*/
class QgsDataSource
{
%TypeHeaderCode
#include <qgsdatasource.h>
%End
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
    /*!\brief Create a database
    * \param path The path where the database file is to be stored
    * \return True if database creation was successful
    */
    bool createDatabase( const QString path );
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
   // bool storeGCPSet( QgsGcpSet );
    /*!\brief Returns the database type
    */
    DatabaseType type();
    /*!\brief Destructor for QgsDataSource
    */
    virtual ~QgsDataSource();
};
