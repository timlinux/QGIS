#ifndef QGSIMAGEPROCESSOR_H
#define QGSIMAGEPROCESSOR_H

#include <QString>
#include <QObject>
#include "qgsimageprocessorthread.h"

using namespace std;

class ANALYSIS_EXPORT QgsImageProcessor : public QObject
{
    Q_OBJECT 
    
  signals:
    void updated(int progress, QString action, int totalProgress);
    void updatedColumns(QString column);
    void updatedLog(QString message);
    
  private slots:
    void update(int progress, QString action, int totalProgress);
    void updateColumns(QString column);
    void updateLog(QString message);

  public:
    QgsImageProcessor();
    ~QgsImageProcessor();
    QgsImageProcessor(const QgsImageProcessor&){}
    void setInputPath(QString path);
    void setOutputPath(QString path);
    void setCheckPath(QString path);
    void setInFilePath(QString path);
    void setOutFilePath(QString path);
    void setThresholds(double gain, double bias);
    void start(int start = -1, int stop = -1);
    void stop();
    bool isRunning();
    void releaseMemory();
    
  private:
    QString mInputPath;
    QString mOutputPath;
    QString mCheckPath;
    QString mInFilePath;
    QString mOutFilePath;
    double mGainThreshold;
    double mBiasThreshold;
    int previousProgress;
    QgsImageProcessorThread *thread;

};


#endif
