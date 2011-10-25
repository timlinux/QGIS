#ifndef QGSBANDALIGNER_H
#define QGSBANDALIGNER_H

#include "qgsprogressmonitor.h"
#include "qgsimagealigner.h"

#include <QString>
#include <QStringList>

class ANALYSIS_EXPORT QgsBandAligner : public QObject
{
    Q_OBJECT

  public:

    enum Transformation // unused
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

    void executeDisparity(QgsProgressMonitor &monitor);

    static void execute(QgsProgressMonitor *monitor, QgsBandAligner *self);
    
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

    inline QString GetDisparityGridPath() { return mDisparityGridPath; }
    inline void SetDisparityGridPath(QString value) { mDisparityGridPath = value; }

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
    QString mDisparityGridPath;
    QStringList mInputPaths;
    QgsBandAligner::Transformation mTransformationType; // unused
    int mMinimumGCPs; // unused
    double mRefinementTolerance; // unused
    QgsProgressMonitor *mMonitor;
};

#endif
