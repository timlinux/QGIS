/*!\ingroup analysis*/
/*! \brief Abstract base class for all sensor models. Provides the
* interface for transforming Ground coordinates to image coordinates
*/
class QgsSensorModel
{
%TypeHeaderCode
#include <qgssensormodel.h>
%End
  public:
    /*!
     * \brief QgsSensorModel destructor
     * Abstract
     */
    virtual ~QgsSensorModel() = 0;
    /*!\brief Returns the normalised row position for pixel at X,Y,Z
     *
     */
    virtual double r( double X, double Y, double Z ) = 0;
    /*!\brief Returns the normalised column position for pixel at X,Y,Z
     */
    virtual double c( double X, double Y, double Z ) = 0;

};

