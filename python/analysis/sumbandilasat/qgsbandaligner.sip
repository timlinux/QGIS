
class QgsBandAligner : QObject
{

%TypeHeaderCode
#include <qgsbandaligner.h>
%End

  signals:
    
    void progressed(double progress);
    void logged(QString message);

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

    QgsBandAligner(QStringList input, QString output, QString disparityXPath = "", QString disparityYPath = "", int blockSize = 201, int referenceBand = 1, QgsBandAligner::Transformation transformation = QgsBandAligner::Disparity, int minimumGcps = 512, double refinementTolerance = 1.9);
    void start();
    void stop();

};
