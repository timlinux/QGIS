
class QgsLineCorrector : QObject
{

%TypeHeaderCode
#include <qgslinecorrector.h>
%End

  public:
    
    enum CorrectionType
    {
      None = 0,
      ColumnBased = 1,
      LineBased = 2
    };

    QgsLineCorrector(QString inputPath, QString outputPath, QString outputRefPath);
    QgsLineCorrector(const QgsLineCorrector&);
    void start();
    void stop();
    void setPhases(QgsLineCorrector::CorrectionType phase1, QgsLineCorrector::CorrectionType phase2, bool recalculate);
    void setCreateMask(bool create);
};
