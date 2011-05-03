/*! \ingroup analysis*/
/*! \brief Container for query results
*/
struct QUERY_RESULT
{
%TypeHeaderCode
#include <qgssqlitedriver.h>
%End
  QList<QString> pHeader;
  QList<QString> pData;
};

/*! \ingroup analysis*/
/*! \brief The driver resposible for the SQLite database connection
 */
class QgsSqliteDriver : QgsSqlDriver
{
%TypeHeaderCode
#include <qgssqldriver.h>
#include <qgssqlitedriver.h>
%End
  public:
    /*!\brief Constructor for QgsSqliteDriver
     * \param path The location of the database
     * \param hashAlgorithm The hash algorithm to be used in the database, defaults to MD5
     */
    QgsSqliteDriver( const QString path, const QgsHasher::HashAlgorithm hashAlgorithm = QgsHasher::Md5 );
    /*!\brief Opens the database for processing
     * \return True on successful open
     */
    bool openDatabase();
    /*!\brief Opens the database for processing based on the database name
     * \param name The name of the database
     * \return True on successful open
     */
    bool openDatabase( const QString path );
    /*!\brief Deletes the active database
     */
    void deleteDatabase();
    /*!\brief Execute a user-defined query
     * \param queryString The user-defined query to be executed on the data in the database
     * \param chip A pointer to a image chip, defaults to NULL
     * \return The QSqlQuery that was executed
     */
    QUERY_RESULT executeQuery( const QString queryString, const QByteArray *chip = NULL );
    /*!\brief Creates a database
     * \return True on successful creation
     */
    bool createDatabase();
    /*!\brief Insert data related to a specific image into the database
     * \param width The width of the image
     * \param height The height of the image
     * \param hash The hash value of the image
     * \param pixelWidthX x component of the pixel width
     * \param pixelWidthY y component of the pixel widht
     * \param pixelHeightX x component of the pixel height
     * \param pixelHeightY y component of the pixel height
     * \param topLeftX x coordinate of the center of the upper left pixel
     * \param topLeftY y coordinate of the center of the upper left pixel
     * \param projection The projection definition
     * \return The ID of the newly inserted image
     */
    int insertImage( int width, int height, QString hash, double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection );
    /*!\brief Insert a reference set into the active database
     * \param id The ID of the set. Checks if the set exists in the database: if it exists, the set is updated, if not, the set is added
     * \param image The image ID
     * \return The ID of the newly inserted Reference Set
     */
    int insertReferenceSet( int id, int image );
    /*!\brief Insert a raw set into the active database
     * \param id The ID of the set. Checks if the set exists in the database: if it exists, the set is updated, if not, the set is added
     */
    int insertRawSet( int id, int image, int referenceSet );
    /*!\brief Insert a reference-image GCP into the active database
     * \param id The ID of the GCP.  Checks if the GCP exists in the database: if it exists, the GCP is updated, of not, the GCP is added
     * \param coordinateX The x-coordinate of the GCP
     * \param coordinateY The y-coordinate of the GCP
     * \param chip The chip extracted from the reference image containing the GCP
     * \param chipWidth The width of the extracted chip
     * \param chipHeight The height of the extracted chip
     * \param chipType The data type of the chip
     * \param chipBands The number of raster bands in the chip
     * \param referenceSet The reference set to which the GCP belongs
     * \return The ID of the newly inserted GCP
     */
    int insertReferenceGcp( int id, double coordinateX, double coordinateY, QByteArray *chip, int chipWidth, int chipHeight, QString chipType, int chipBands, int referenceSet );
    /*!\brief Insert a raw-image GCP into the active database
     * \param id TThe ID of the GCP.  Checks if the GCP exists in the database: if it exists, the GCP is updated, of not, the GCP is added
     * \param coordinateX The x-coordinate of the GCP
     * \param coordinateY The y-coordinate of the GCP
     * \param referenceGCP The reference GCP from which this raw-image GCP was cross-referenced from
     * \param The raw set to which the GCP belongs
     * \return The ID of the newly inserted GCP
     */
    int insertRawGcp( int id, double coordinateX, double coordinateY, int referenceGcp, int rawSet );
    /*!\brief Select an entire GCP Dataset based on the hash values of the raw and reference images
     * \param refHash The hash value of the reference image
     * \param rawHash The hash value of the raw image
     * \return A QgsGcpSet populated with the relevant GCPs based on the provided hash values
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
    /*!\brief Closes the active database
     */
    void closeDatabase();
    /*!\brief Reconnects to the last active database by closing the connection and re-opening it again
     */
    bool reconnect();
    /*!\brief Returns the last error if one occured
     */
    QString lastError();
    /*!\brief Retrieve the value from a query result
      * \param results The results used to retrieve the value
      * \param position The position of the value
      * \return The value at the given position
      */
    QString queryValue(QUERY_RESULT results, int position);
    /*!\brief Retrieve the value from a query result
      * \param results The results used to retrieve the value
      * \param header The header of the column the value should be retrieved from
      * \return The value in a given column
      */
    QString queryValue(QUERY_RESULT results, QString header);
};
