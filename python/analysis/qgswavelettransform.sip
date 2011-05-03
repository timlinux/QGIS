%Import QtCore/QtCoremod.sip 
class QgsWaveletTransform
{
%TypeHeaderCode
#include <qgswavelettransform.h>
%End

public:
  typedef QList<QgsSalientPoint> PointsList;
  QgsWaveletTransform( QgsRasterDataset* image);
	~QgsWaveletTransform();
  bool execute();
  PointsList* extractDistributed(int quantity) const;
  PointsList* extract(int quantity, int column, int row, int offset = 0) const;
  const PointsList* constList(unsigned int column,unsigned int row) const;
  bool initialized();
  void setRasterBand( int band );
  int rasterBand();
  void setFeatureSize( int xSize, int ySize );
  void setMaxFeatures( int value );
  void setOrdering( bool value );
  void setDistribution( int columns, int rows );
  void distribution(int& columns, int& rows);
  void suggestDistribution(int nNumPoints);
  void setMaxBlackCoverage( double value );
  double maxBlackCoverage()const ;
};
