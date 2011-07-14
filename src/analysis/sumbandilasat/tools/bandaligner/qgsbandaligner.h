#ifndef QGSBANDALIGNER_H
#define QGSBANDALIGNER_H

#include <QString>
#include <QThread>
#include <QObject>
#include <QStringList>
#include <QList>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <gdal_priv.h>
#include <iostream>
#include "qgsimagealigner.h"

using namespace std;

class ANALYSIS_EXPORT QgsBandAlignerThread;

class ANALYSIS_EXPORT QgsBandAligner : public QObject
{
  
    Q_OBJECT
  
  signals:
  
    void logged(QString message);
    void progressed(double progress);
    
  public slots:
  
    void logReceived(QString message);
    void progressReceived(double progress);
  
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
    ~QgsBandAligner();
    void start();
    void stop();
    
  private:
    QgsBandAlignerThread *mThread;
};
  
class ANALYSIS_EXPORT QgsBandAlignerThread : public QThread
{
  
    Q_OBJECT
  
  signals:
  
    void logged(QString message);
    void progressed(double progress);
  
  public:
    
    QgsBandAlignerThread(QStringList input, QString output, QString disparityXPath = "", QString disparityYPath = "", int blockSize = 201, int referenceBand = 1, QgsBandAligner::Transformation transformation = QgsBandAligner::Disparity, int minimumGcps = 512, double refinementTolerance = 1.9);
    ~QgsBandAlignerThread();
    void run();
    void stop();
    
  private:
    
    int mReferenceBand;
    QStringList mInputPath;
    QString mOutputPath;
    QString mDisparityXPath;
    QString mDisparityYPath;
    int mBlockSize;
    GDALDataset *mInputDataset;
    GDALDataset *mOutputDataset;
    int mBands;
    int mWidth;
    int mHeight;
    bool mStopped;
    QgsBandAligner::Transformation mTransformation;
    int mMinimumGcps;
    double mRefinementTolerance;
};

#endif