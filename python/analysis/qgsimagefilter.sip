/*! \ingroup analysis*/
/*! \brief  Abstract base class for any filter that can be applied to an image
*/
class QgsImageFilter
{
%TypeHeaderCode
#include <qgsimagefilter.h>
%End
  public:
    /*! \brief Applies this filter to the given image*/
    virtual QgsRasterDataset* applyFilter() = 0;
    QString destinationFile() const ;
    void setDestinationFile( QString path );
    protected:
    QgsImageFilter( QgsRasterDataset* image );

};

