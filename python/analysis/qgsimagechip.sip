/*! \ingroup analysis*/
/*! \brief Subclass of the QgsRasterDataset that enables in-memory storage
* of the image data
*/

class QgsImageChip : QgsRasterDataset
{
%TypeHeaderCode
#include <qgsrasterdataset.h>
#include <qgsimagechip.h>
%End
  public:    
    /*!
     * \brief Creates an in-memory raster with the given properies
     * \param width The width in pixels of the in-memory raster
     * \param height The height in pixels
     * \param type The data type of the pixels in the dataset
     * \param bands The amount of bands of the new dataset
     * \return Returns the constructed dataset or NULL on failure
     */
    static QgsImageChip* createImageChip( int width, int height, GDALDataType type,  int bands = 3 );
    /*!
     * \brief Creates an in-memory raster with from an existing data buffer
     * The data buffer must have enough space allocated to hold
     * (width * height * bands) elements of the given type.
     * \param width The width in pixels of the in-memory raster
     * \param height The height in pixels
     * \param type The data type of the pixels in the dataset
     * \param bands The amount of bands of the new dataset
     * \param data A pointer to the start of the to be used for the dataset
     * \return Returns the constructed dataset or NULL on failure
     */
    static QgsImageChip* createImage( int width, int height, GDALDataType type, int bands, void* data );
    ~QgsImageChip();
    QgsImageChip( const QgsImageChip& other );
    /*!\brief Returns a pointer to the in-memory data
     */
    void* data();
    /*!\brief Returns the size in bytes of the in-memory data buffer
     *
     */
    long dataSize();
};
