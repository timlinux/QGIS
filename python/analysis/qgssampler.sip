/*!\ingroup analysis*/
/*! \brief Abstract base class of all image (re)samplers
* An image sampler is used to sample pixels from an image based on coordinates that are not exact grid coordinates
*/
class QgsSampler /Abstract/
{
%TypeHeaderCode
#include <qgssampler.h>
%End
  public:
    virtual double samplePoint( QgsRasterDataset* image, int band, double pixel, double line ) = 0;
};

