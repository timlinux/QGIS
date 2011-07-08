#include "qgsmemoryraster.h"

QgsRasterReadThread::QgsRasterReadThread(QString *path, int *columns, int *rows, int *bands, QList<double**> *theData, bool *success)
{
  mPath = path;
  mColumns = columns;
  mRows = rows;
  mBands = bands;
  mProgress = 0.0;
  data = theData;
  mSuccess = success;
  wasStopped = false;
}

void QgsRasterReadThread::run()
{
  mProgress = 0.0;
  if (0 == GDALGetDriverCount())
  {
    GDALAllRegister();
  }
  GDALDataset *dataset = (GDALDataset*) GDALOpen(mPath->toAscii().data(), GA_ReadOnly);
  if(dataset)
  {
    *mBands = dataset->GetRasterCount();
    *mColumns = dataset->GetRasterXSize();
    *mRows = dataset->GetRasterYSize();
    double progressAddition = 100.0/((*mBands**mRows) + (*mBands**mRows**mColumns));
    for(int j = 1; j <= *mBands && !wasStopped; j++)
    {
      int **theData = new int*[*mRows];
      for(int i = 0; i < *mRows && !wasStopped; i++)
      {
	int *rasterData = new int[*mColumns];
	dataset->GetRasterBand(j)->RasterIO(GF_Read, 0, i, *mColumns, 1, rasterData, *mColumns, 1, GDT_UInt32, 0, 0);
	theData[i] = rasterData;
	mProgress += progressAddition;
      }
      double **theNewData = new double*[*mColumns];
      
      //Do this loop, otherwise when the users stops the thread, not all indexs will be assigned to a value. When deleteing this index a segemntation fault will occur.
      for(int col = 0; col < *mColumns; col++)
      {
	theNewData[col] = NULL;
      }
      
      for(int col = 0; col < *mColumns && !wasStopped; col++)
      {
	double *rasterData = new double[*mRows];
	for(int row = 0; row < *mRows; row++)
	{
	   rasterData[row] = (double) theData[row][col];
	   mProgress += progressAddition;
	}
	theNewData[col] = rasterData;
      }
      
      //clear memory
      for(int i = 0; i < *mRows; i++)
      {
	delete [] theData[i];
      }
      delete [] theData;
      theData = NULL;
      data->append(theNewData);
    }
    *mSuccess = true;
  }
  else
  {
    *mSuccess = false;
  }
  //delete dataset;
  GDALClose(dataset);
  dataset = NULL;
  //GDALClose((GDALDatasetH) dataset); //can't close dataset, because we previously deleted bandData
  mProgress = 100.0;
}

void QgsRasterReadThread::stop()
{
  wasStopped = true;
}

QgsRasterWriteThread::QgsRasterWriteThread(QString *path, QString outputPath, int *columns, int *rows, int *bands, QList<double**> *theData, bool *success)
{
  mOutputPath = outputPath;
  mPath = path;
  mColumns = columns;
  mRows = rows;
  mBands = bands;
  mProgress = 0.0;
  data = theData;
  mSuccess = success;
  wasStopped = false;
}

void QgsRasterWriteThread::run()
{
  mProgress = 0.0;
  if (0 == GDALGetDriverCount())
  {
    GDALAllRegister();
  }
  mProgress += 5.0;
  GDALDataset *oldSet = (GDALDataset*) GDALOpen(mPath->toAscii().data(), GA_ReadOnly);
  mProgress += 5.0;
  mProgress += 5.0;
  GDALDataset *newSet = oldSet->GetDriver()->CreateCopy(mOutputPath.toAscii().data(), oldSet, false, NULL, NULL, NULL);
  mProgress += 25.0;
  double progressAddition = 60.0/(*mBands**mRows**mColumns);
  for(int band = 1; band <= *mBands && !wasStopped; band++)
  {
    double** allData = data->at(band-1);
    for(int i = 0; i < *mRows && !wasStopped; i++)
    {
      int *writeData = new int[*mColumns];
      for(int j = 0; j < *mColumns && !wasStopped; j++)
      {
	writeData[j] = (int) allData[j][i];
	mProgress += progressAddition;
      }
      newSet->GetRasterBand(band)->RasterIO(GF_Write, 0, i, *mColumns, 1, writeData, *mColumns, 1, GDT_UInt32, 0, 0);
      newSet->GetRasterBand(band)->FlushCache();
      delete [] writeData;
    }
    /*for(int i = 0; i < *mColumns; i++)
    {
      delete [] allData[i];
      allData[i] = NULL;
    }
    delete allData;
    allData = NULL;*/
    //delete bandData;
  }
  /*delete data;
  data = NULL;*/
  
  newSet->FlushCache();
  mProgress = 100.0;
  GDALClose(oldSet);
  //delete oldSet;
  oldSet = NULL;
  GDALClose(newSet);
  //delete newSet;
  newSet = NULL;
}

void QgsRasterWriteThread::stop()
{
  wasStopped = true;
}

QgsRasterCheckThread::QgsRasterCheckThread(QString in, QString out, QString outputPath, int *columns, int *rows, int *bands, bool *success)
{
  mOutputPath = outputPath;
  mInPath = in;
  mOutPath = out;
  mColumns = columns;
  mRows = rows;
  mBands = bands;
  mProgress = 0.0;
  mSuccess = success;
  wasStopped = false;
}

void QgsRasterCheckThread::run()
{
  mProgress = 0.0;
  if (0 == GDALGetDriverCount())
  {
    GDALAllRegister();
  }
  mProgress += 5.0;
  GDALDataset *oldSet1 = (GDALDataset*) GDALOpen(mInPath.toAscii().data(), GA_ReadOnly);
  GDALDataset *oldSet2 = (GDALDataset*) GDALOpen(mOutPath.toAscii().data(), GA_ReadOnly);
  mProgress += 5.0;
  GDALDriver *driver = oldSet1->GetDriver();
  mProgress += 5.0;
  GDALDataset *newSet = driver->CreateCopy(mOutputPath.toAscii().data(), oldSet1, true, NULL, NULL, NULL);
  mProgress += 25.0;
  double progressAddition = 60.0/(*mBands**mRows**mColumns);
  for(int band = 1; band <= *mBands && !wasStopped; band++)
  {
    GDALRasterBand *bandData1 = oldSet1->GetRasterBand(band);
    GDALRasterBand *bandData2 = oldSet2->GetRasterBand(band);
    GDALRasterBand *bandData = newSet->GetRasterBand(band);

    for(int i = 0; i < *mRows && !wasStopped; i++)
    {
      int *readData1 = (int*) CPLMalloc(sizeof(int)*(*mColumns));
      bandData1->RasterIO(GF_Read, 0, i, *mColumns, 1, readData1, *mColumns, 1, GDT_UInt32, 0, 0);
      int *readData2 = (int*) CPLMalloc(sizeof(int)*(*mColumns));
      bandData2->RasterIO(GF_Read, 0, i, *mColumns, 1, readData2, *mColumns, 1, GDT_UInt32, 0, 0);
      int *writeData = (int*) CPLMalloc(sizeof(int)*(*mColumns));
      
      
      for(int j = 0; j < *mColumns; j++)
      {
	writeData[j] = readData1[j] - readData2[j];
	mProgress += progressAddition;
      }
      bandData->RasterIO(GF_Write, 0, i, *mColumns, 1, writeData, *mColumns, 1, GDT_UInt32, 0, 0);
      bandData->FlushCache();
      delete [] readData1;
      delete [] readData2;
      delete [] writeData;
    }
  }
  newSet->FlushCache();
  GDALClose(oldSet1);
  GDALClose(oldSet2);
  GDALClose(newSet);
  mProgress = 100.0;
}

void QgsRasterCheckThread::stop()
{
  wasStopped = true;
}








QgsMemoryRaster::QgsMemoryRaster(QString path)
{
  mPath = path;
  thread = NULL;
}

QgsMemoryRaster::~QgsMemoryRaster()
{
  for(int i = 0; i < data.size(); i++)
  {
    for(int j = 0; j < mColumns; j++)
    {
      if(data[i][j] != NULL)
      {
	delete [] data[i][j];
	data[i][j] = NULL;
      }
    }
    if(data[i] != NULL)
    {
      delete [] data[i];
      data[i] = NULL;
    }
  }
  data.clear();
  data = QList<double**>();
  if(thread != NULL)
  {
    delete thread;
    thread = NULL;
  }
}

double* QgsMemoryRaster::column(int band, int index)
{
  return data[band-1][index];
}

void QgsMemoryRaster::setColumn(int band, int index, double* newData)
{
  for(int i = 0; i < mRows; i++)
  {
    data[band-1][index][i] = newData[i];
  }
  /*delete [] data[band-1][index];
  data[band-1][index] = newData;*/
}

int QgsMemoryRaster::getRows()
{
  return mRows;
}
  
int QgsMemoryRaster::getColumns()
{
  return mColumns;
}

int QgsMemoryRaster::getBands()
{
  return mBands;
}

void QgsMemoryRaster::open()
{
  if(thread != NULL)
  {
    delete thread;
  }
  thread = new QgsRasterReadThread(&mPath, &mColumns, &mRows, &mBands, &data, &mSuccess);
  thread->start();
}

void QgsMemoryRaster::writeCopy(QString path)
{
  if(thread != NULL)
  {
    delete thread;
  }
  thread = new QgsRasterWriteThread(&mPath, path, &mColumns, &mRows, &mBands, &data, &mSuccess);
  thread->start();
}

void QgsMemoryRaster::writeCheck(QString corrected, QString path)
{
  if(thread != NULL)
  {
    delete thread;
  }
  thread = new QgsRasterCheckThread(mPath, corrected, path, &mColumns, &mRows, &mBands, &mSuccess);
  thread->start();
}

double QgsMemoryRaster::progress()
{
  return thread->progress();//mProgress;
}

bool QgsMemoryRaster::success()
{
  return mSuccess;
}

bool QgsMemoryRaster::isRunning()
{
  if (thread != NULL)
  {
    return thread->isRunning();
  }
  return false;
}

void QgsMemoryRaster::stop()
{
  if(thread != NULL)
  {
    thread->stop();
  }
}
