#ifndef QGSMEMORYLINERASTER_H
#define QGSMEMORYLINERASTER_H

#include <QString>
#include <QList>
#include <QThread>
#include <gdal_priv.h>
#include <iostream>

using namespace std;

//abstract class
class QgsRasterLineThread : public QThread
{
  public:
    
    virtual int bandCount(){return -1;}
    virtual int columnCount(){return -1;}
    virtual int rowCount(){return -1;}
    virtual void stop() = 0;
    virtual int progress() = 0;
};

class QgsRasterReadLineThread : public QgsRasterLineThread
{
      
  public:
    QgsRasterReadLineThread(QString path, QList<int**> *theData);
    void run();
    void stop();
    
    int bandCount(){return mBands;}
    int columnCount(){return mColumns;}
    int rowCount(){return mRows;}
    int progress(){return mProgress;}
    
  private:
    QString mPath;
    int mColumns;
    int mRows;
    int mBands;
    QList<int**> *data;
    bool wasStopped;
    double mProgress;
    
};

class QgsRasterWriteLineThread : public QgsRasterLineThread
{
      
  public:
    QgsRasterWriteLineThread(QString path, QString outputPath, QList<int**> *theData);
    void run();
    void stop();
    
    int bandCount(){return mBands;}
    int columnCount(){return mColumns;}
    int rowCount(){return mRows;}
    int progress(){return mProgress;}
    
  private:
    QString mOutputPath;
    QString mPath;
    int mColumns;
    int mRows;
    int mBands;
    QList<int**> *data;
    bool wasStopped;
    double mProgress;
    
};

class QgsMemoryLineRaster : public QObject
{
      Q_OBJECT
  
  public:
    
    QgsMemoryLineRaster(QString path);
    ~QgsMemoryLineRaster();
    
    int* column(int band, int index);
    void setColumn(int band, int index, int* newData);
    
    int rowCount();
    int columnCount();
    int bandCount();
    
    void open();
    void save(QString path);
    
    bool isRunning();
    void stop();
    
  signals:
    void progressed(double progress);
    
  private:
    QString mPath;
    int mColumns;
    int mRows;
    int mBands;
    QList<int**> data;
    QgsRasterLineThread *thread;
};

#endif
