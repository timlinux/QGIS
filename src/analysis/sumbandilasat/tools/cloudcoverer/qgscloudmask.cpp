#include "qgscloudmask.h"

QgsCloudMask::QgsCloudMask(int rows, int cols, GDALDataset *dataset)
{
  mRows = rows;
  mCols = cols;
  mData = new bool*[rows];
  for(int i = 0; i < rows; i++)
  {
    mData[i] = new bool[cols];
    for(int j = 0; j < cols; j++)
    {
      mData[i][j] = false;
    }
  }
  mDataset = dataset;
  mCloudColor = QColor(190,238,44);
  mUnitSize = 0.5;
}

QgsCloudMask::~QgsCloudMask()
{
  for(int i = 0; i < mRows; i++)
  {
    delete [] mData[i];
  }
  delete [] mData;
}

bool QgsCloudMask::value(int row, int col)
{
  return mData[row][col];
}

void QgsCloudMask::setValue(int row, int col, bool value)
{
  mData[row][col] = value;
}

int QgsCloudMask::rows()
{
  return mRows;
}

int QgsCloudMask::columns()
{
  return mCols;
}

QgsVectorLayer* QgsCloudMask::vectorLayer()
{
  QgsVectorLayer *mask = new QgsVectorLayer("Point?crs=epsg:4326", "cloud_mask", "memory");
  QgsVectorDataProvider *pr = mask->dataProvider();
  
  QgsFeatureList l;
  for(int i = 0; i < mRows; i++)
  {
    for(int j = 0; j < mCols; j++)
    {
      if(mData[i][j])
      {
	QgsFeature fet;
	fet.setGeometry(QgsGeometry::fromPoint(QgsPoint(j,-i)));
	l.append(fet);

      }
    }
  }
  pr->addFeatures( l);
  
  QgsMarkerSymbolV2 *symbol = new QgsMarkerSymbolV2();
  symbol->deleteSymbolLayer(0);
  symbol->appendSymbolLayer( new QgsSimpleMarkerSymbolLayerV2("circle", mCloudColor, mCloudColor));
  symbol->setColor(QColor(190,238,44));
  symbol->setSize(mUnitSize);
  QgsSingleSymbolRendererV2 *renderer = new QgsSingleSymbolRendererV2( symbol );
  mask->setRendererV2(renderer);

  return mask;
}

void QgsCloudMask::setCloudColor(QColor color)
{
  mCloudColor = color;
}

void QgsCloudMask::setUnitSize(double size)
{
  mUnitSize = size;
}
