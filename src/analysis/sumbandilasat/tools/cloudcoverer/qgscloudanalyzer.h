#ifndef QGSCLOUDANALYZER_H
#define QGSCLOUDANALYZER_H

#include <QString>
#include <QList>
#include <QColor>
#include <gdal_priv.h>
#include "limits.h"

#include "qgscloudmask.h"

class QgsCloudAnalyzer
{

  public:
    
    QgsCloudAnalyzer(QString path, int threshold = 250);
    QgsCloudMask* analyze();
    long highestValue();
    long lowestValue();
    void setThreshold(int value);
    int percentage();
    void setBandUse(int bandNumber);
    void setCloudColor(QColor color);
    void setUnitSize(double size);
    
  protected:
    void calculateValues(); //Highest and lowest
    
  private:
    
    GDALDataset *mDataset;
    int mThreshold;
    int mBands;
    int mCols;
    int mRows;
    long mHighest;
    long mLowest;
    int mPercentage;
    int mBandUse;
    QColor mCloudColor;
    double mUnitSize;
};


#endif
