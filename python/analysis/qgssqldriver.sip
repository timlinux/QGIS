/*! \ingroup analysis*/
/*! \brief The abstract driver resposible for the SQL database connection
 */
class QgsSqlDriver
{
%TypeHeaderCode
#include <qgssqldriver.h>
%End
  public:
    /*! \brief Opens the database for processing
    * \return True on successful open
    */
    virtual bool openDatabase() = 0;//Opens the database
    /*!\brief Opens the database for processing based on the database name
     * \param name The name of the database
     * \return True on successful open
     */
    virtual bool createDatabase() = 0;
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
    virtual int insertImage( int width, int height, QString hash, double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection ) = 0;
    /*!\brief Insert a reference set into the active database
     * \param id The ID of the set. Checks if the set exists in the database: if it exists, the set is updated, if not, the set is added
     * \param image The image ID
     * \return The ID of the newly inserted Reference Set
     */
    virtual int insertReferenceSet( int id, int image ) = 0;
    /*!\brief Insert a raw set into the active database
     * \param id The ID of the set. Checks if the set exists in the database: if it exists, the set is updated, if not, the set is added
     */
    virtual int insertRawSet( int id, int image, int referenceSet ) = 0;
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
    virtual int insertReferenceGcp( int id, double coordinateX, double coordinateY, QByteArray *chip, int chipWidth, int chipHeight, QString chipType, int chipBands, int referenceSet ) = 0;
    /*!\brief Insert a raw-image GCP into the active database
     * \param id TThe ID of the GCP.  Checks if the GCP exists in the database: if it exists, the GCP is updated, of not, the GCP is added
     * \param coordinateX The x-coordinate of the GCP
     * \param coordinateY The y-coordinate of the GCP
     * \param referenceGCP The reference GCP from which this raw-image GCP was cross-referenced from
     * \param The raw set to which the GCP belongs
     * \return The ID of the newly inserted GCP
     */
    virtual int insertRawGcp( int id, double coordinateX, double coordinateY, int referenceGcp, int rawSet ) = 0;
    /*!\brief Select an entire GCP Dataset based on the hash values of the raw and reference images
     * \param refHash The hash value of the reference image
     * \param rawHash The hash value of the raw image
     * \return A QgsGcpSet populated with the relevant GCPs based on the provided hash values
     */
    virtual QgsGcpSet* selectByHash( const QString refHash, const QString rawHash ) = 0;
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
    virtual QgsGcpSet* selectByLocation( double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection ) = 0;
    /*!\brief Check if the database is open
     */
    bool isOpen();
    /*!\brief Set the hasing algorithm to be used in the active database
     * \param hashAlgorithm The hashing algorithm to be used
     */
    void setHashAlgorithm( QgsHasher::HashAlgorithm hashAlgorithm );
    /*!\brief Return the hashing algorithm that is used in the active database
     */
    QgsHasher::HashAlgorithm hashAlgorithm();
    /*!\brief QgsSqlDriver destructor
     */
    ~QgsSqlDriver();
};
