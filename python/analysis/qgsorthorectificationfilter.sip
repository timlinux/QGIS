/*! \ingroup analysis*/

class QgsOrthorectificationFilter : QgsImageFilter
{
%TypeHeaderCode
#include <qgsorthorectificationfilter.h>
#include <qgsimagefilter.h>
%End
  public:
   
   
    QgsOrthorectificationFilter( QgsRasterDataset* image, QgsSensorModel* sensormodel, QgsSampler* sampler, QgsElevationModel* DEM = NULL);
   ~QgsOrthorectificationFilter();
    QgsRasterDataset* applyFilter();

    /**************************************************
     *  Getters and Setters
     **************************************************/
    QgsSampler* sampler() const;
    QgsSensorModel* sensorModel() const;
    void setSampler( QgsSampler* sampler );
    QString driver()const;
    void setDriver( QString name );

};

