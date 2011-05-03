/*! \ingroup analysis*/
/*! \brief General purpose raster dataset and reader that provides access to the metadata of the image.
*/
class GDALDataset;
class GDALDataType;
class QgsRasterDataset
{
%TypeHeaderCode
#include <qgsrasterdataset.h>
%End
  public:
    enum Access
    {
      ReadOnly = 0,
      Update = 1
    };
   
    /**\brief QgsRasterDataset Constructor
     * Constructs the dataset with the given path and initializes all internal data
     */
    QgsRasterDataset( QString path,  Access access, bool automaticallyOpen);
    /*! \brief QgsRasterDataset Constructor
     * Constructs the dataset with the given GDAL dataset.
     */
    QgsRasterDataset( GDALDataset* dataset );
    /*! \brief QgsRasterDataset Destructor
     * Destroys the QgsRasterDataset instance, releasing any owned resources.
     */
    virtual ~QgsRasterDataset();
    /*! \brief Returns a pointer to the internal GDALDataset instance or NULL if initialization failed
     */
    GDALDataset* gdalDataset();
    /*! \brief Returns true if this object failed to initialize with the given file
     */
    virtual bool failed() const;
    /*! \brief Gets the width and height of the raster image in pixels
     * Returns the width through the xSize reference parameter and the height through ySize.
     * If the image did not load or initialize correctly it does nothing.
     * Use failed() to check validity of the object.
     */
    void imageSize( int& xSize, int& ySize ) const;

    /*! \brief Gets the width of this image in pixels
     */
    int imageXSize() const;
    /*! \brief Gets the height of this image in pixels
     */
    int imageYSize() const;

    /*! \brief Gets the number of raster bands in the image.
     * Returns the number of rasterbands in the image or 0 if it hasn't been initialized.
     */
    int rasterBands() const;

    /*! \brief Reads the value of the image at a specified point
     */
    double readValue( int band, int xPixel, int yLine, bool* succeeded );
    /*! \brief Checks whether this object has initialized correctly
     */
    operator bool() const;
    /*! \brief Gets the file path of the image on disk represented by this dataset
     */
    QString filePath() const;

    /*! \brief Returns the Geographic coordinates of the top-left corner of the image
     */
    void geoTopLeft( double& X, double& Y ) const;
    /*! \brief Returns the Geographic coordinates of the bottom-right corner of the image
     */
    void geoBottomRight( double& X, double& Y ) const;
     static const int TM_PIXELGEO; //FROM PIXEL to GEO-COORDS
    static const int TM_GEOPIXEL;//FROM GEO to PIXEL COORDS
    /*! \brief Transforms coordinates between image and ground space using affine transformation coefficients
     * \param pixelX The x-coordinate in pixels
     * \param pixelY The y-coordinate in pixels
     * \param geoX The ground x-coordinate
     * \param geoY The ground y-coordinate
     * \param mode Flag indicating whether the ground coordinates should be calculated from image coordinates or vice versa.
     */
    bool transformCoordinate( int& pixelX, int& pixelY, double& geoX, double& geoY, const int mode ) const;

    /*!\brief Gets the data type of this dataset
     * \return Returns the GDALDataType of this image dataset
     */
    GDALDataType rasterDataType() const;
    /*! \brief Checks whether this image has georeferencing information set.
     * \return Returns true if the image has 6 georeferencing coefficients set and false otherwise
     */
    bool georeferenced() const;
    /*! \brief Sets the Georeference coefficients of this image
     * \param topLeftX The x-coordinate of the upper left corner of the image
     * \param pixelWidthX The x-component of the pixel width
     * \param pixelWidthY The y-component of the pixel width (in the case of rotated images)
     * \param topLeftY The y-coordinate of the upper left corner of the image
     * \param pixelHeightX The x-component of the pixel height (in the case of rotated images)
     * \param pixelHeightY The y-component of the pixel height
     */
    void setGeoCoefficients( double topLeftX, double pixelWidthX, double pixelWidthY, double topLeftY, double pixelHeightX, double pixelHeightY );
    /*! \brief Sets the Georeference coefficients of this image.
     * The coefficients must be given in the following order:
     * [0] : The x-coordinate of the upper left corner of the image
     * [1] : The x-component of the pixel width
     * [2] : The y-component of the pixel width (in the case of rotated images)
     * [3] : The y-coordinate of the upper left corner of the image
     * [4] : The x-component of the pixel height (in the case of rotated images)
     * [5] : The y-component of the pixel height
     * \param coeff A pointer to an array containing the coefficients.
     */
    void setGeoCoefficients( double* coeff );
    /*! \brief Gets the georeferencing coefficients of the image
     * See setGeoCoefficients(double* coeff)
     */
    bool geoCoefficients( double* coeff ) const;
    /*! \brief Loads an ESRI World file with georeferencing information for this dataset
     * The coefficients in the world file will replace any existing coefficients.
     * Call georeferenced() to check whether the dataset already has coefficients
     * \return Returns true on success or false if the world file contained invalid data or the information could not be set.
     */
    bool loadWorldFile( QString filename );

    /*! \brief Set the projection reference string for this dataset
     */
    bool setProjection( QString projString );
    /*! \brief Fetch the projection definition string for this dataset
     */
    QString projection() const;
};
