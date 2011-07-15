
class QgsCloudMask
{

%TypeHeaderCode
#include <qgscloudmask.h>
%End

  public:
    
    QgsCloudMask(int rows, int cols);
    ~QgsCloudMask();
    bool value(int row, int col);
    void setValue(int row, int col, bool value);
    int rows();
    int columns();
    QgsVectorLayer* vectorLayer();

};
