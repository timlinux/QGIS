#include "qgscloudanalyzer.h"

QgsCloudAnalyzer::QgsCloudAnalyzer(QString path, int threshold)
{
  mThreshold = threshold;
  if (GDALGetDriverCount() == 0)
  {
    GDALAllRegister();
  }
  mDataset = (GDALDataset*) GDALOpen(path.toAscii().data(), GA_ReadOnly);
  if(mDataset)
  {
    mBands = mDataset->GetRasterCount();
    mCols = mDataset->GetRasterXSize();
    mRows = mDataset->GetRasterYSize();
  }
  else
  {
    mBands = -1;
    mCols = -1;
    mRows = -1;
  }
  mHighest = 0;
  mLowest = LONG_MAX;
  mPercentage = 0;
  mBandUse = 1;
  mCloudColor = QColor(190,238,44);
  mUnitSize = 0.5;
  calculateValues();
}

void QgsCloudAnalyzer::calculateValues()
{
  if(mDataset)
  {
    GDALRasterBand *band = mDataset->GetRasterBand(mBandUse);
    for(int i = 0; i < mRows; i++)
    {
      int *data = new int[mCols];
      band->RasterIO(GF_Read, 0, i, mCols, 1, data, mCols, 1, GDT_UInt32, 0, 0);
      for(int j = 0; j < mCols; j++)
      {
        if(data[j] > mHighest)
        {
          mHighest = data[j];
        }
        if(data[j] < mLowest)
        {
          mLowest = data[j];
        }
      }
      delete [] data;
    }
  }
}

QgsCloudMask* QgsCloudAnalyzer::analyze()
{
  QgsCloudMask *mask = new QgsCloudMask(mRows, mCols, mDataset);
  mask->setCloudColor(mCloudColor);
  mask->setUnitSize(mUnitSize);
  if(!mDataset)
  {
    return mask;
  }
  GDALRasterBand *band = mDataset->GetRasterBand(mBandUse);
  long counter = 0;
  for(int i = 0; i < mRows; i++)
  {
    int *data = new int[mCols];
    band->RasterIO(GF_Read, 0, i, mCols, 1, data, mCols, 1, GDT_UInt32, 0, 0);
    for(int j = 0; j < mCols; j++)
    {
      if(data[j] > mThreshold)
      {
        counter++;
        mask->setValue(i, j, true);
      }
      else
      {
        mask->setValue(i, j, false);
      }
    }
    delete [] data;
  }
  mPercentage = (int) ((counter*1.0)/(mRows*mCols)*100.0);
  return mask;
}

long QgsCloudAnalyzer::highestValue()
{
  return mHighest;
}

long QgsCloudAnalyzer::lowestValue()
{
  return mLowest;
}

void QgsCloudAnalyzer::setThreshold(int value)
{
  mThreshold = value;
}

int QgsCloudAnalyzer::percentage()
{
  return mPercentage;
}

void QgsCloudAnalyzer::setBandUse(int bandNumber)
{
  mBandUse = bandNumber;
}

void QgsCloudAnalyzer::setCloudColor(QColor color)
{
  mCloudColor = color;
}

void QgsCloudAnalyzer::setUnitSize(double size)
{
  mUnitSize = size;
}
