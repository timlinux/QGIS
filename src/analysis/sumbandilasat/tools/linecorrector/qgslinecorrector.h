#ifndef QGSLINECORRECTOR_H
#define QGSLINECORRECTOR_H

#define ROW_OUTLIERS 1
#define COL_OUTLIERS 0.5
#define STANDARD_DEVIATION_COL 15
#define STANDARD_DEVIATION_ROW 15
#define STANDARD_DEVIATION 0.0224549199347
#define MEAN 0

#include <QString>
#include <QThread>
#include <QFile>
#include <gdal_priv.h>

#include "qgsmemorylineraster.h"
#include "qgslogger.h"

struct LineErrors
{
  QList<bool> up;
  QList<bool> down;
};

class QgsLineCorrectorThread;

class QgsLineCorrector : public QObject
{
    Q_OBJECT
  
  public:
    
    enum CorrectionType
    {
      ColumnBased = 0,
      LineBased = 1,
      None = 2
    };
    
    QgsLineCorrector(QString inputPath, QString outputPath, QString outputRefPath);
    QgsLineCorrector(const QgsLineCorrector&){}
    void start();
    void setPhases(QgsLineCorrector::CorrectionType phase1, QgsLineCorrector::CorrectionType phase2, bool recalculate);
    void setCreateMask(bool create);
    void stop();
    
  private:
    
    QString mInputPath;
    QString mOutputPath;
    QString mOutputRefPath;
    QgsLineCorrectorThread *thread;
    
  public slots:
    void progress(double total, double part, QString description);
    void log(QString value);
    void findBadLines(QList<int> value);
    
  signals:
    void progressed(double total, double part, QString description);
    void logged(QString value);
    void badLinesFound(QList<int> value);
};

class QgsLineCorrectorThread : public QThread
{
    Q_OBJECT
  
  public:
    
    QgsLineCorrectorThread(QObject *parent = NULL);
    void setPaths(QString inputPath, QString outputPath, QString outputRefPath);
    void setProcess1(QgsLineCorrector::CorrectionType type);
    void setProcess2(QgsLineCorrector::CorrectionType type);
    void setRecalculation(bool value);
    void setCreateMask(bool create);
    void run();
    void stop();
    
  signals:
    void progressed(double total, double part, QString description);
    void logged(QString value);
    void badLinesFound(QList<int> value);
    
  protected:
    
    void openImages();
    void closeImages();
    int* readLine(int lineNumber);
    int* readOutLine(int lineNumber);
    int* readColumn(int columnNumber);
    int* readOutColumn(int columnNumber);
    
    QList<int> detectErrors(bool fromOutput = false);
    void fixErrors(QList<int> badColumns);
    void fixColumn(QList<int> badColumns, QString path);
    void fixLine(QList<int> badColumns);
    void createMask();
    
    void correct(int *column, QList<int> corrupt);
    int* calculateMask(QList<int> corrupt, int size);
    LineErrors findErrors(QList<double> means, int limit, bool logIt = false);
    bool test(double x, double y, int limit);
    double ratio(double a, double b);
    double mean(int *values);
    QList<int> analyzeFailures(QList<bool> failUp, QList<bool> failDown, int limit);
    bool matchPattern0(int row, QList<bool> d, QList<bool> u);
    bool matchPattern1(int row, QList<bool> d, QList<bool> u);
    bool matchPattern2(int row, QList<bool> d, QList<bool> u);
    bool matchPattern3(int row, QList<bool> d, QList<bool> u);
    bool matchPattern4(int row, QList<bool> d, QList<bool> u);
    bool matchPattern5(int row, QList<bool> d, QList<bool> u);
    bool matchPattern6(int row, QList<bool> d, QList<bool> u);
    bool matchPattern7(int row, QList<bool> d, QList<bool> u);
    bool matchPattern8(int row, QList<bool> d, QList<bool> u);
    bool matchPattern9(int row, QList<bool> d, QList<bool> u);
    bool matchPattern10(int row, QList<bool> d, QList<bool> u);
    bool matchPattern11(int row, QList<bool> d, QList<bool> u);
    bool matchPattern12(int row, QList<bool> d, QList<bool> u);
    bool matchPattern13(int row, QList<bool> d, QList<bool> u);
    bool matchPattern14(int row, QList<bool> d, QList<bool> u);
    
  public slots:
    void readColumnRasterProgressed(double progress);
    void writeColumnRasterProgressed(double progress);
    
  private:
    
    QString mInputPath;
    QString mOutputPath;
    QString mOutputRefPath;
    GDALDataset *mInputImage;
    GDALDataset *mOutputImage;
    GDALDataset *mOutputRefImage;
    int mColumns;
    int mRows;
    QgsLineCorrector::CorrectionType mType1;
    QgsLineCorrector::CorrectionType mType2;
    bool mRecalculate;
    bool mCreateMask;
    QString mCurrentAction;
    double mProgressDevider;
    double mProgress;
    QList<int> mBadColumns;
    bool mWasStopped;
};


#endif
