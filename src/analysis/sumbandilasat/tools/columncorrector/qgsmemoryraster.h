#ifndef QGSMEMORYRASTER_H
#define QGSMEMORYRASTER_H

#include <QString>
#include <QList>
#include <QThread>
#include <gdal_priv.h>
#include <iostream>

using namespace std;

//abstract class
class ANALYSIS_EXPORT QgsRasterThread : public QThread
{
  public:
    virtual void stop() = 0;
    virtual double progress(){return 0;}
};

class ANALYSIS_EXPORT QgsRasterReadThread : public QgsRasterThread
{
  public:
    QgsRasterReadThread(QString *path, int *columns, int *rows, int *bands, QList<double**> *theData, bool *success);
    void run();
    void stop();
    double progress(){return mProgress;}
    
  private:
    QString *mPath;
    int *mColumns;
    int *mRows;
    int *mBands;
    double mProgress;
    QList<double**> *data;
    bool *mSuccess;
    bool wasStopped;
};

class ANALYSIS_EXPORT QgsRasterWriteThread : public QgsRasterThread
{
  public:
    QgsRasterWriteThread(QString *path, QString outputPath, int *columns, int *rows, int *bands, QList<double**> *theData, bool *success);
    double progress(){return mProgress;}
    void run();
    void stop();
    
  private:
    QString mOutputPath;
    QString *mPath;
    int *mColumns;
    int *mRows;
    int *mBands;
    double mProgress;
    QList<double**> *data;
    bool *mSuccess;
    bool wasStopped;
};

class ANALYSIS_EXPORT QgsRasterCheckThread : public QgsRasterThread
{
  public:
    QgsRasterCheckThread(QString in, QString out, QString outputPath, int *columns, int *rows, int *bands, bool *success);
    void run();
    void stop();
    double progress(){return mProgress;}
    
  private:
    QString mOutputPath;
    QString mInPath;
    QString mOutPath;
    int *mColumns;
    int *mRows;
    int *mBands;
    double mProgress;
    bool *mSuccess;
    bool wasStopped;
};

class ANALYSIS_EXPORT QgsMemoryRaster
{
  public:
    QgsMemoryRaster(QString path);
    ~QgsMemoryRaster();
    double* column(int band, int index);
    void setColumn(int band, int index, double* newData);
    int getRows();
    int getColumns();
    int getBands();
    void open();
    void writeCopy(QString path);
    void writeCheck(QString corrected, QString path);
    double progress();
    bool success();
    bool isRunning();
    void stop();
    
  protected:
    
    void print(QString s)
    {
      const char *ptr = s.toAscii().data();
      cout << ptr << endl;
    }
    
  private:
    QString mPath;
    int mColumns;
    int mRows;
    int mBands;
    double mProgress;
    bool mSuccess;
    QList<double**> data;
    QgsRasterThread *thread;
};

#endif
