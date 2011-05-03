/*! \ingroup analysis*/
/*! \brief The ImageAnalyzer class is responsible for the physical
* analysis and processing of image data. This included detecting and extracting
* GCPs and GCP chips from images, as well as searching for these reference
* GCP chips on a raw image.
*/
class QgsImageAnalyzer
{
%TypeHeaderCode
#include <qgsimageanalyzer.h>
%End
  public:
    /*! \brief QgsImageAnalyzer Constructor
     * Constructs an analyzer for the supplied image, ready to perform processing on this image.
     */
    QgsImageAnalyzer( QgsRasterDataset* image = 0 );
    /*! \brief Extracts ground control points from the assigned image.
     * This method allocates a new QgsGcpSet and transfers ownership to the caller.
     * The GCP's contained in this set will have geographic reference coordinates and a usable Chip for matching.
     * If this method fails it returns NULL
    */
    QgsGcpSet* extractGcps( int amount );
    /*! \brief Matches the supplied GCP set to the image dataset, using the GCP chips.
     * If the method was successful the supplied GCP set will have it's raw coordinates updated.
     * \param A pointer to the GCP set to use and update.
     * \return A pointer to the supplied GCP set, with updated coordinates, or NULL on failure
     */
    QgsGcpSet* matchGcps( QgsGcpSet* gcpSet );

    /*! \brief QgsImageAnalyzer Destructor
     */
    virtual ~QgsImageAnalyzer();

    /*! \brief Extracts an area of this image analyzer's dataset into a new dataset.
     * The size of the area to be extracted must be specified, as well as the point on the source image to start copying from.
     * The data is read into the destination dataset or, if NULL is specified a new in-memory raster is created for the chip.
     * The datatype of the extracted chip will be the same
     * \param sourcePoint The x, y pixel coordinates on the source image to start copying from.
     * \param xPixels The width in pixels of the area to extract.
     * \param yLines The height in lines of the area to extract.
     * \param bands The number of bands to extract. If set to -1 the method will extract as many as the source dataset has.
     * \param type The datatype of the destination chip. If unknown it defaults to source datatype.
     * \param dest The destination dataset.
     */
    QgsRasterDataset* extractChip( QgsPoint& sourcePoint, int xPixels, int yLines, int bands = -1, GDALDataType type = UNKNOWN_TYPE, QgsRasterDataset* dest = NULL );

    //*****************GETTERS & SETTERS****************************************************
    /*! \brief Sets the image dataset of this analyzer.
     * This can be used to change the image between operations, to enable reuse of the same QgsImageAnalyzer object.
     */
    void setImage( QgsRasterDataset* image );
    QgsRasterDataset* image() const;
    void setChipSize( int xSize, int ySize );
    int chipXSize() const;
    int chipYSize() const;
    void setChipBandCount( int bands );
    int chipBandCount() const ;
    GDALDataType chipDataType()const ;
    void setChipDataType( GDALDataType type );
    double progress()const;
    void setProgress(double value);
    double correlationThreshold()const;
    void setCorrelationThreshold( double value );
    //********************************************************************************************
};
