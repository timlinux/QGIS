/*! \ingroup analysis*/
/*! \brief Class for handling the cross correlation of two subimages
*/
class QgsCrossCorrelator
{
%TypeHeaderCode
#include <qgscrosscorrelator.h>
%End
  public:
    /**\brief QgsCrossCorrelator Constructor
     * Constructs the correlator with the given chip images
    */
    QgsCrossCorrelator( QgsRasterDataset* ref, QgsRasterDataset* raw );
    /**\brief QgsCrossCorrelator Destructor
     * Destroys the QgsCrossCorrelator instance, releasing any owned resources.
    */
    virtual ~QgsCrossCorrelator();
    double correlationValue();
    typedef  int Scheme;
};
