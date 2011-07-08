#ifndef QGSCLOUDMASK_H
#define QGSCLOUDMASK_H

#include "qgslogger.h"
#include "qgsvectorlayer.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorfilewriter.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgssymbolv2.h"
#include "qgssinglesymbolrendererv2.h"
#include "qgsmarkersymbollayerv2.h"

#include <QDir>
#include <QColor>

#include <gdal_priv.h>

class QgsCloudMask
{

  public:
    
    QgsCloudMask(int rows, int cols, GDALDataset *dataset = NULL);
    ~QgsCloudMask();
    bool value(int row, int col);
    void setValue(int row, int col, bool value);
    int rows();
    int columns();
    QgsVectorLayer* vectorLayer();
    void setCloudColor(QColor color);
    void setUnitSize(double size);
    
  private:
    
    bool** mData;
    int mCols;
    int mRows;
    GDALDataset *mDataset;
    QColor mCloudColor;
    double mUnitSize;
};


#endif
