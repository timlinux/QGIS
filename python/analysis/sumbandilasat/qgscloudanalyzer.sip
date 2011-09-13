
class QgsCloudAnalyzer
{

%TypeHeaderCode
#include <qgscloudanalyzer.h>
%End

  public:
    
    QgsCloudAnalyzer(QString path, int threshold = 250);
    QgsCloudMask* analyze();
    long highestValue();
    long lowestValue();
    void setThreshold(int value);
    int percentage();
    void setBandUse(int bandNumber);
    void setCloudColor(QColor color);
    void setUnitSize(double size);
};
