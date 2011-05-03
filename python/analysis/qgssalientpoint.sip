/*! \ingroup analysis*/
/*! \brief This class represents any point of interest on a two-dimensional raster image.
*/
class QgsSalientPoint : QgsPoint
{
%TypeHeaderCode
#include <qgspoint.h>
#include <qgssalientpoint.h>
%End
  public:
    /*! \brief QgsSalientPoint constructor
     * constructs a feature point at the given x, y coordinates and optionally a saliency value.
    */
    QgsSalientPoint( double x = 0, double y = 0, double theSaliency = 0 );
    /*! \brief Gets the Saliency value of this feature.
    */
    double saliency()const;
    /*! \brief Sets the Saliency value of this feature.
    */
    void setSaliency( double value );
};
