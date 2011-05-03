/*! \ingroup analysis
 * This class encapsulates a Rational Polynomial Coefficients
 * model, or RFM (Rational Function Model), which is constructed using GCPs, hence
 * it is terrain-dependent. Its major public methods return values indicating the
 * orthorectified normalised pixel position for each pixel in the original image.
 */
class QgsRpcModel : QgsSensorModel
{
%TypeHeaderCode
#include <qgsrpcmodel.h>
#include <qgssensormodel.h>
%End
  public:
    QgsRpcModel( QgsGcpSet* gcpSet, QgsRasterDataset* img, QgsElevationModel* dem );  
    virtual ~QgsRpcModel();   
    double r( double X, double Y, double Z );  
    double c( double X, double Y, double Z );
};
