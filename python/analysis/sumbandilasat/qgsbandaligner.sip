
class QgsBandAligner : QObject
{

%TypeHeaderCode
#include <qgsbandaligner.h>
%End

  public:

    enum Transformation
    {
      Disparity = 0,
      ThinPlateSpline = 1,
      Polynomial2 = 2,
      Polynomial3 = 3,
      Polynomial4 = 4,
      Polynomial5 = 5
    };
    
    QgsBandAligner(QStringList inputs, QString output);

    void ClearInputPaths();
    void AppendInputPath(QString value);
    void SetOutputPath(QString value);
    void SetDisparityXPath(QString value);
    void SetDisparityYPath(QString value);
    void SetReferenceBand(int value);
    void SetBlockSize(int value);
    void SetTransformationType(QgsBandAligner::Transformation value);
    void SetMinimumGCPs(int value);
    void SetRefinementTolerance(double value);   
    void SetProgressMonitor(QgsProgressMonitor *monitor);

    void start();
    void stop();
};
