/*! \ingroup analysis*/
/*! \brief Class for sampling an image using bilinear interpolation
*/
class QgsBilinearSampler : QgsSampler
{
%TypeHeaderCode
#include <qgsbilinearsampler.h>
#include <qgssampler.h>
%End
  public:
    /*! \brief Samples an image at a given location using bilinear interpolation
     * Edge pixels are treated as exceptions and uses either linear filtering or nearest neighbour
     * \param image A pointer to the image to sample from
     * \param band The band inside the image to sample from
     * \param pixel The x coordinate in pixels
     * \param line The y-coordinate in pixels
     * \return The sampled value obtained through the necessary interpolation or 0.0 if the pixel/line coordinate is out of bounds
     */
    double samplePoint( QgsRasterDataset* image, int band, double pixel, double line )const;


};

