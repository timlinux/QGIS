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
#include "qgsprogressmonitor.h"

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

    QgsBandAligner(QStringList inputs,  QString output)
      : mBlockSize(201), 
        mReferenceBand(1), 
        mTransformationType(QgsBandAligner::Disparity),
        mMinimumGCPs(512),
        mRefinementTolerance(1.9),
        mMonitor(NULL)
    {
        mInputPaths = inputs;
        mOutputPath = output;
    }
    ~QgsBandAligner() {}

    void start();
    void stop();

    static void QgsBandAligner::execute(QgsProgressMonitor *monitor, QgsBandAligner *self);
    
    void executeDisparity(QgsProgressMonitor &monitor);
    void executeWarp(QgsProgressMonitor &monitor);

  //private:
  //  QgsBandAlignerThread *mThread;

  public:      
    inline int GetBlockSize() { return mBlockSize; }
    inline void SetBlockSize(int value) { mBlockSize = value; };

    inline int GetReferenceBand() { return mReferenceBand; }
    inline void SetReferenceBand(int value) { mReferenceBand = value; };

    inline QString GetOutputPath() { return mOutputPath; }
    inline void SetOutputPath(QString value) { mOutputPath = value; }

    inline QString GetDisparityXPath() { return mDisparityXPath; }
    inline void SetDisparityXPath(QString value) { mDisparityXPath = value; }

    inline QString GetDisparityYPath() { return mDisparityYPath; }
    inline void SetDisparityYPath(QString value) { mDisparityYPath = value; }

    inline QStringList GetInputPaths() { return mInputPaths; }
    inline void AppendInputPath(QString value) { mInputPaths.append(value); }
    inline void ClearInputPaths() { mInputPaths.clear(); }

    inline QgsBandAligner::Transformation GetTransformationType() { return mTransformationType; }
    inline void SetTransformationType(QgsBandAligner::Transformation value) { mTransformationType = value; }

    inline int GetMinimumGCPs() { return mMinimumGCPs; }
    inline void SetMinimumGCPs(int value) { mMinimumGCPs = value; }

    inline double GetRefinementTolerance() { return mRefinementTolerance; }
    inline void SetRefinementTolerance(double value) { mRefinementTolerance = value; }

    inline void SetProgressMonitor(QgsProgressMonitor *value) { mMonitor = value; }

  private:
    int mBlockSize;
    int mReferenceBand;
    QString mOutputPath;
    QString mDisparityXPath;
    QString mDisparityYPath;
    QStringList mInputPaths;
    QgsBandAligner::Transformation mTransformationType;
    int mMinimumGCPs;
    double mRefinementTolerance;
    QgsProgressMonitor *mMonitor;
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
    void run_disparity(QgsProgressMonitor &monitor);
    void run_gdalwarp();
    
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
