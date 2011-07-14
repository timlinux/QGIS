
class QgsImageProcessor : QObject
{

%TypeHeaderCode
#include <qgsimageprocessor.h>
%End

  signals:
    void updated(int, QString, int);
    void updatedColumns(QString column);
    void updatedLog(QString message);

  public:

    QgsImageProcessor();
    ~QgsImageProcessor();
    QgsImageProcessor(const QgsImageProcessor&);
    void setInputPath(QString);
    void setOutputPath(QString);
    void setCheckPath(QString);
    void setInFilePath(QString);
    void setOutFilePath(QString);
    void setThresholds(double, double);
    void start(int, int);
    void stop();
    bool isRunning();
    void releaseMemory();

  private:

    void operator=(const QgsImageProcessor&);
};
