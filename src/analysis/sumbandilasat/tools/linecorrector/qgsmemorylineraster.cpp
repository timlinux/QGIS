#include "qgsmemorylineraster.h"

QgsRasterReadLineThread::QgsRasterReadLineThread(QString path, QList<int**> *theData)
{
  mPath = path;
  mColumns = 0;
  mRows = 0;
  mBands = 0;
  data = theData;
  wasStopped = false;
}

void QgsRasterReadLineThread::run()
{
  if (0 == GDALGetDriverCount())
  {
    GDALAllRegister();
  }
  
  GDALDataset *dataset = (GDALDataset*) GDALOpen(mPath.toAscii().data(), GA_ReadOnly);
  if(dataset)
  {
    mBands = dataset->GetRasterCount();
    mColumns = dataset->GetRasterXSize();
    mRows = dataset->GetRasterYSize();
    for(int j = 1; j <= mBands && !wasStopped; j++)
    {
      int **theData = new int*[mRows];
      GDALRasterBand *bandData = dataset->GetRasterBand(j);
      for(int i = 0; i < mRows && !wasStopped; i++)
      {
        int *rasterData = new int[mColumns];
	bandData->RasterIO(GF_Read, 0, i, mColumns, 1, rasterData, mColumns, 1, GDT_UInt32, 0, 0);
	theData[i] = rasterData;
	
	mProgress = (double(i)*double(j))/(double(mRows)*double(mBands))*50.0;
      }
      int **theNewData = new int*[mColumns];
      
      //Do this loop, otherwise when the users stops the thread, not all indexs will be assigned to a value. When deleteing this index a segemntation fault will occur.
      for(int col = 0; col < mColumns; col++)
      {
	theNewData[col] = NULL;
      }
      
      for(int col = 0; col < mColumns && !wasStopped; col++)
      {
        int *rasterData = new int[mRows];
	for(int row = 0; row < mRows; row++)
	{
	   rasterData[row] = theData[row][col];
	   mProgress = 50.0+((double(col)*double(j))/(double(mRows)*double(mBands))*50.0);
	}
	theNewData[col] = rasterData;
      }
      
      //clear memory
      for(int i = 0; i < mRows; i++)
      {
	delete [] theData[i];
      }
      delete [] theData;
      theData = NULL;
      data->append(theNewData);
    }
    //emit finished(QgsRasterThread::Successful);
  }
  else
  {
    //emit finished(QgsRasterThread::Failed);
  }
  mProgress = 100.0;
  GDALClose(dataset);
  dataset = NULL;
}

void QgsRasterReadLineThread::stop()
{
  wasStopped = true;
}

QgsRasterWriteLineThread::QgsRasterWriteLineThread(QString path, QString outputPath, QList<int**> *theData)
{
  mOutputPath = outputPath;
  mPath = path;
  mColumns = 0;
  mRows = 0;
  mBands = 0;
  mProgress = 0.0;
  data = theData;
  wasStopped = false;
}


void QgsRasterWriteLineThread::run()
{
  mProgress = 0.0;
  if (0 == GDALGetDriverCount())
  {
    GDALAllRegister();
  }
  mProgress = 5.0;
  GDALDataset *oldSet = (GDALDataset*) GDALOpen(mPath.toAscii().data(), GA_ReadOnly);
  mBands = oldSet->GetRasterCount();
  mColumns = oldSet->GetRasterXSize();
  mRows = oldSet->GetRasterYSize();
  mProgress = 10.0;
  GDALDriver *driver = oldSet->GetDriver();
  mProgress = 15.0;
  GDALDataset *newSet = driver->CreateCopy(mOutputPath.toAscii().data(), oldSet, true, NULL, NULL, NULL);
  mProgress = 40.0;
  for(int band = 1; band <= mBands && !wasStopped; band++)
  {
    GDALRasterBand *bandData = newSet->GetRasterBand(band);
    int** allData = data->at(band-1);
    for(int i = 0; i < mRows && !wasStopped; i++)
    {
      int *writeData = new int[mColumns];
      for(int j = 0; j < mColumns && !wasStopped; j++)
      {
	writeData[j] = allData[j][i];
      }
      mProgress = 40.0+((double(i)*double(band))/(double(mBands)*double(mRows)))*60.0;
      bandData->RasterIO(GF_Write, 0, i, mColumns, 1, writeData, mColumns, 1, GDT_UInt32, 0, 0);
      bandData->FlushCache();
      delete [] writeData;
    }
    //delete bandData;
  }
  newSet->FlushCache();
  mProgress = 100.0;
  GDALClose(oldSet);
  oldSet = NULL;
  GDALClose(newSet);
  newSet = NULL;
}

void QgsRasterWriteLineThread::stop()
{
  wasStopped = true;
}

QgsMemoryLineRaster::QgsMemoryLineRaster(QString path)
{
  mPath = path;
  thread = NULL;
}

QgsMemoryLineRaster::~QgsMemoryLineRaster()
{
  for(int i = 0; i < data.size(); i++)
  {
    for(int j = 0; j < mColumns; j++)
    {
      if(data.at(i)[j] != NULL)
      {
	delete [] data.at(i)[j];
	data.at(i)[j] = NULL;
      }
    }
    delete [] data.at(i);
  }
  data.clear();
  if(thread != NULL)
  {
    delete thread;
    thread = NULL;
  }
}

int* QgsMemoryLineRaster::column(int band, int index)
{
  return data.at(band-1)[index];
}

void QgsMemoryLineRaster::setColumn(int band, int index, int* newData)
{
  data.at(band-1)[index] = newData;
}

int QgsMemoryLineRaster::rowCount()
{
  return mRows;
}

int QgsMemoryLineRaster::columnCount()
{
  return mColumns;
}

int QgsMemoryLineRaster::bandCount()
{
  return mBands;
}

void QgsMemoryLineRaster::open()
{
  if(thread != NULL)
  {
    delete thread;
  }
  thread = new QgsRasterReadLineThread(mPath, &data);
  thread->start();
  int progress = 0;
  while(thread->isRunning())
  {
    int newProgress = (int) thread->progress();
    if(progress != newProgress)
    {
      progress = newProgress;
      //emit progressed(thread->progress());
    }
  }
  mBands = thread->bandCount();
  mColumns = thread->columnCount();
  mRows = thread->rowCount();
}

void QgsMemoryLineRaster::save(QString path)
{
  if(thread != NULL)
  {
    delete thread;
  }
  thread = new QgsRasterWriteLineThread(mPath, path, &data);
  thread->start();
  int progress = 0;
  while(thread->isRunning())
  {
    int newProgress = (int) thread->progress();
    if(progress != newProgress)
    {
      progress = newProgress;
      //emit progressed(thread->progress());
    }
  }
}

bool QgsMemoryLineRaster::isRunning()
{
  if (thread != NULL)
  {
    return thread->isRunning();
  }
  return false;
}

void QgsMemoryLineRaster::stop()
{
  if(thread != NULL)
  {
    thread->stop();
  }
}
