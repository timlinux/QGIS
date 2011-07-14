#ifndef QGSIMAGEPROCESSORTHREAD_H
#define QGSIMAGEPROCESSORTHREAD_H

#include <QString>
#include <QFile>
#include <QThread>
#include <QList>
#include <QQueue>
#include <gsl/gsl_fit.h>
#include <gdal_priv.h>
#include <iostream>
#include "qgsmemoryraster.h"

using namespace std;

struct BadColumn
{
  int column;
  double m;
  double c;
  bool fault; //if fault = 0, then the column is over the gain threshold, otherwise it is over the bias threshold
};

class ANALYSIS_EXPORT QgsReadingThread : public QThread
{
  public:
    QgsReadingThread(bool leftRight, int start, int stop, int band, int rows, int columns, QgsMemoryRaster *dataset, QList<BadColumn*> *potentialBadLeft, QList<BadColumn*> *potentialBadRight); //if leftRight = 0, then run from left to right, else run from right to left
    void setThresholds(double gainThreshold, double biasThreshold);
    double progress();
    QString logMessage();
    bool hasLogMessage();
    void run();
    void stop();
    
  protected:
    void readFromLeft(int band, int start, int stop);
    void readFromRight(int band, int start, int stop);
    
  private:
    double mProgress;
    bool mLeftRight;
    int mBand;
    int mColumns;
    int mRows;
    int mStart;
    int mStop;
    double mGainThreshold;
    double mBiasThreshold;
    QgsMemoryRaster *mDataset;
    QList<BadColumn*> *mPotentialBadLeft;
    QList<BadColumn*> *mPotentialBadRight;
    QQueue<QString> log;
    bool wasStopped;
};

class ANALYSIS_EXPORT QgsCorrectionThread : public QThread
{
  public:
    QgsCorrectionThread(int start, int stop, int band, int rows, int columns, QgsMemoryRaster *dataset, QList< QList<BadColumn*> > *finalFaults, QgsMemoryRaster *newRaster);
    void setThresholds(double gainThreshold, double biasThreshold);
    void run();
    double progress();
    QString logMessage();
    bool hasLogMessage();
    void stop();
    
  private:
    double mProgress;
    int mBand;
    int mColumns;
    int mRows;
    int mStart;
    int mStop;
    QgsMemoryRaster *mDataset;
    double mGainThreshold;
    double mBiasThreshold;
    QList< QList<BadColumn*> > *mFinalFaults;
    QgsMemoryRaster *mOutset;
    QQueue<QString> log;
    bool wasStopped;
};

class ANALYSIS_EXPORT QgsQualityTester
{
  public:
    static int goodQuality(double m, double c, double thresholdM, double thresholdC); //0 = bad on gain, 1 = bad on bias, 2 = good
    
  protected:
    static bool highVariance(double base, double threshold, double value);
};

class ANALYSIS_EXPORT QgsImageProcessorThread : public QThread
{
    Q_OBJECT
    
  public:
    QgsImageProcessorThread(QString inputPath, QString outputPath, QString checkPath, QString inPath, QString outPath, double gainThreshold, double biasThreshold, int start = -1, int stop = -1);
    QgsImageProcessorThread(const QgsImageProcessorThread&){}
    ~QgsImageProcessorThread();
    void run();
    void stop();
    bool stopped();
    void releaseMemory();
    
  protected:
    bool openDataset(QString inputPath);
    
    void calculateFaults(int band);
    
    void correctFaults();
    void correctBand(int band, QgsMemoryRaster *newRaster);
    
    void createCheck();
    
    bool writeToFile(QString path);
    bool readFromFile(QString path);
    
    void handleProgress(double progress, QString action);
    void handleLog(QString message);

  signals:
    void updated(int progress, QString action, int totalProgress);
    void updatedColumns(QString column);
    void updatedLog(QString message);
    
  private:
    QString mInputPath;
    QString mOutputPath;
    QString mCheckPath;
    QString mInFilePath;
    QString mOutFilePath;
    double mGainThreshold;
    double mBiasThreshold;
    
    QgsMemoryRaster *dataset;
    QgsMemoryRaster *outputDataset;
    int bands;
    int columns;
    int rows;
    
    int mStart;
    int mStop;
    
    int previousProgress;
    int totalProgress;
    int totalTasks;
    int totalProgressBase;
    QString mProgressAction;
    
    QList<BadColumn*> potentialBadLeft;
    QList<BadColumn*> potentialBadRight;
    QList< QList<BadColumn*> > finalFaults;
    
    QgsReadingThread *rt1;
    QgsReadingThread *rt2;
    QgsCorrectionThread *ct1;
    QgsCorrectionThread *ct2;
    bool wasStopped;
};

#endif
