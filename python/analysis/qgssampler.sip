/*! \ingroup analysis*/
/*! \brief Class for sampling an image using bilinear interpolation
*/
class QgsSampler
{
%TypeHeaderCode
#include <qgssampler.h>
%End
  public:
    
    double samplePoint( QgsRasterDataset* image, int band, double pixel, double line );


};

