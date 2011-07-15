#include "qgsimagealigner.h"

QgsImageAligner::QgsImageAligner()
{
  mRealDisparity = NULL;
  mInputDataset = NULL;
  mTranslateDataset = NULL;
  mDisparityReal = NULL;
  mDisparityImag = NULL;
}

QgsImageAligner::QgsImageAligner(QString inputPath, QString translatePath, int referenceBand, int unreferencedBand, int blockSize)
{
  mInputPath = inputPath;
  mTranslatePath = translatePath;
  mReferenceBand = referenceBand;
  mUnreferencedBand = unreferencedBand;
  if(blockSize % 2 != 0)
  {
      mBlockSize = blockSize;
  }
  else
  {
      //mBlockSize = blockSize + 1;
      mBlockSize = blockSize - 1;
  }
  mRealDisparity = NULL;
  mInputDataset = NULL;
  mTranslateDataset = NULL;
  mDisparityReal = NULL;
  mDisparityImag = NULL;
  mWidth = 0;
  mHeight = 0;
  mMax = 0;
  mBits = 255;
  open();
}

QgsImageAligner::~QgsImageAligner()
{
  if(mRealDisparity != NULL)
  {
    for(int i = 0; i < mHeight; i++)
    {
      delete [] mRealDisparity[i];
    }
    delete [] mRealDisparity;
    mRealDisparity = NULL;
  }
  if(mDisparityReal != NULL)
  {
    free(mDisparityReal);
    mDisparityReal = NULL;
  }
  if(mDisparityImag != NULL)
  {
    free(mDisparityImag);
    mDisparityImag = NULL;
  }
  if(mInputDataset != NULL)
  {
    GDALClose(mInputDataset);
    mInputDataset = NULL;
  }
  if(mTranslateDataset != NULL)
  {
    GDALClose(mTranslateDataset);
    mTranslateDataset = NULL;
  }
}

void QgsImageAligner::open()
{
  if (GDALGetDriverCount() == 0)
  {
    GDALAllRegister();
  }
  mInputDataset = (GDALDataset*) GDALOpen(mInputPath.toAscii().data(), GA_ReadOnly);
  mTranslateDataset = (GDALDataset*) GDALOpen(mTranslatePath.toAscii().data(), GA_ReadOnly);
  mWidth = mInputDataset->GetRasterXSize();
  mHeight = mInputDataset->GetRasterYSize();
  mBits = pow(2, (double)GDALGetDataTypeSize(mInputDataset->GetRasterBand(1)->GetRasterDataType()));
}

void QgsImageAligner::eliminate(double sigmaWeight)
{
  int xSteps = ceil(mWidth / double(mBlockSize));
  int ySteps = ceil(mHeight / double(mBlockSize));
  double *x = new double[xSteps*ySteps];
  double *y = new double[xSteps*ySteps];
  
  QgsComplex *newFixed = new QgsComplex[xSteps*ySteps];
  for(int i = 0; i < xSteps*ySteps; i++)
  {
    newFixed[i] = QgsComplex(0.0, 0.0);
  }
    
  int xCounter = 0;
  int yCounter = 0;
  for(int j = 0; j < xSteps; j++)
  {
    for(int i = 0; i < ySteps; i++)
    {
	//if(fixed[xCounter].real() != 0.0) mRealDisparity[i][j].setReal(fixed[xCounter].real());
	x[xCounter] = mReal[i][j].real();
	xCounter++;
	//if(fixed[yCounter].imag() != 0.0) mRealDisparity[i][j].setImag(fixed[yCounter].imag());
	y[yCounter] = mReal[i][j].imag();
	yCounter++;
    }
  }
  
  double stdDeviationX = gsl_stats_sd(x, 1, xSteps);
  double stdDeviationY = gsl_stats_sd(y, 1, ySteps);
  double meanX = gsl_stats_mean(x, 1, xSteps);
  double meanY = gsl_stats_mean(y, 1, ySteps);
  int counter = 0;
  
  for(int j = 0; j < xSteps; j++)
  {
    for(int i = 0; i < ySteps; i++)
    {
	if(isBad(mReal[i][j].real(), stdDeviationX, meanX, sigmaWeight))
	{
	  double avg = 0.0;
	  int avgCounter = 0;
	  if(i-1 >= 0 && !isBad(mReal[i-1][j].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i-1][j].real(); avgCounter++;}
	  if(j-1 >= 0 && !isBad(mReal[i][j-1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i][j-1].real(); avgCounter++;}
	  if(i+1 < ySteps && !isBad(mReal[i+1][j].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i+1][j].real(); avgCounter++;}
	  if(j+1 < xSteps && !isBad(mReal[i][j+1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i][j+1].real(); avgCounter++;}
	  if(i+1 < ySteps && j+1 < xSteps && !isBad(mReal[i+1][j+1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i+1][j+1].real(); avgCounter++;}
	  if(i-1 >= 0 && j+1 < xSteps && !isBad(mReal[i-1][j+1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i-1][j+1].real(); avgCounter++;}
	  if(i+1 < ySteps && j-1 >= 0 && !isBad(mReal[i+1][j-1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i+1][j-1].real(); avgCounter++;}
	  if(i-1 >= 0 && j-1 >= 0 && !isBad(mReal[i-1][j-1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i-1][j-1].real(); avgCounter++;}
	  mReal[i][j].setReal(avg / avgCounter);
	  newFixed[counter].setReal(mReal[i][j].real());
	}
	if(isBad(mReal[i][j].imag(), stdDeviationY, meanY, sigmaWeight))
	{
	  double avg = 0.0;
	  int avgCounter = 0;
	  if(i-1>=0 && !isBad(mReal[i-1][j].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i-1][j].imag(); avgCounter++;}
	  if(j-1>=0 && !isBad(mReal[i][j-1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i][j-1].imag(); avgCounter++;}
	  if(i+1< ySteps && !isBad(mReal[i+1][j].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i+1][j].imag(); avgCounter++;}
	  if(j+1< xSteps && !isBad(mReal[i][j+1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i][j+1].imag(); avgCounter++;}
	  if(i+1 < ySteps && j+1 < xSteps && !isBad(mReal[i+1][j+1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i+1][j+1].imag(); avgCounter++;}
	  if(i-1 >= 0 && j+1 < xSteps && !isBad(mReal[i-1][j+1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i-1][j+1].imag(); avgCounter++;}
	  if(i+1 < ySteps && j-1 >= 0 && !isBad(mReal[i+1][j-1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i+1][j-1].imag(); avgCounter++;}
	  if(i-1 >= 0 && j-1 >= 0 && !isBad(mReal[i-1][j-1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i-1][j-1].imag(); avgCounter++;}
	  mReal[i][j].setImag(avg / avgCounter);
	  newFixed[counter].setImag(mReal[i][j].imag());
	}
	counter++;
    }
  }
  
  int c1 = 0;
  int c2 = 0;
  for(int j = 0; j < mWidth; j++)
  {
    for(int i = 0; i < mHeight; i++)
    {
      if(mRealDisparity[i][j].real() != 0.0 && mRealDisparity[i][j].real() == mRealDisparity[i][j].real() && mRealDisparity[i][j].imag() != 0.0 && mRealDisparity[i][j].imag() == mRealDisparity[i][j].imag())
      {
	mRealDisparity[i][j] = mReal[c1][c2];
	c1++;
	if(c1>=ySteps)
	{
	  c2++;
	  c1 = 0;
	}
      }
    }
  }
  delete [] x;
  delete [] y;
}

bool QgsImageAligner::isBad(double value, double deviation, double mean, double sigmaWeight)
{
  if(value > mean+(sigmaWeight*deviation) || value < mean-(sigmaWeight*deviation))
  {
    return true;
  }
  return false;
}

void QgsImageAligner::scan()
{
  double xSteps = ceil(mWidth / double(mBlockSize));
  double ySteps = ceil(mHeight / double(mBlockSize));
  double xDiff = (xSteps * mBlockSize - mWidth) / (xSteps - 1);
  double yDiff = (ySteps * mBlockSize - mHeight) / (ySteps - 1);
  mRealDisparity = new QgsComplex*[mHeight];
  for(int i = 0; i < mHeight; i++)
  {
    mRealDisparity[i] = new QgsComplex[mWidth];
    for(int j = 0; j < mWidth; j++)
    {
      mRealDisparity[i][j] = QgsComplex();
    }
  }
  
  mReal = new QgsComplex*[int(ySteps)];
  for(int i = 0; i < int(ySteps); i++)
  {
    mReal[i] = new QgsComplex[int(xSteps)];
    for(int j = 0; j < int(xSteps); j++)
    {
      mReal[i][j] = QgsComplex();
    }
  }
  
  QMutex mutex;
  QgsComplex c;
  
  int xCounter = 0;
  for(int x = 0; x < xSteps; x++)
  {
    int yCounter = 0;
    int newX = floor((x + 0.5) * mBlockSize - x * xDiff);
    for(int y = 0; y < ySteps; y++)
    {
      int newY = floor((y + 0.5) * mBlockSize - y * yDiff);
      double point[2];
      point[0] = newX;
      point[1] = newY;
      QgsComplex c;
      mRealDisparity[newY][newX] = findPoint(point, c);
      mReal[yCounter][xCounter] = mRealDisparity[newY][newX];
      /*if(mReal[yCounter][xCounter].real() == 0.0 && mReal[yCounter][xCounter].imag() == 0.0)
      {
	mSnrTooLow.append(QgsComplex(yCounter, xCounter));
      }*/
      yCounter++;
    }
    xCounter++;
  }
  GDALClose(mInputDataset);
  mInputDataset = (GDALDataset*) GDALOpen(mInputPath.toAscii().data(), GA_ReadOnly);
}

QgsComplex QgsImageAligner::findPoint(double *point, QgsComplex estimate)
{
  double newPoint[2];
  newPoint[0] = point[0] + estimate.real();
  newPoint[1] = point[1] + estimate.imag();
  double corrX = estimate.real();
  double corrY = estimate.imag();
  QgsRegion *region1 = region(mInputDataset, point, mBlockSize, mReferenceBand);
  QgsRegion *region2 = region(mTranslateDataset, newPoint, mBlockSize, mUnreferencedBand);

  double val = 0;
  for(int i = 0; i < region2->width*region2->height; i++)
  {
    val += region2->data[i];
  }
  
  //If chip is water
  /*if(mBits-val/(region2->width*region2->height)< 55)
  {
    cout<<"water ("<<point[0]<<", "<<point[1]<<")"<<endl;
    delete [] region1->data;
    delete [] region2->data;
    delete region1;
    delete region2;
    return QgsComplex();
  }*/
  
  QgsRegionCorrelationResult corrData = QgsRegionCorrelator::findRegion(region1, region2, mBits);
  
  delete [] region1->data;
  delete [] region2->data;
  delete region1;
  delete region2;
  if(corrData.wasSet)
  {
    corrX = corrData.x;
    corrY = corrData.y;
    //cout<<"Point found("<<point[0]<<" "<<point[1]<<"): "<<corrX<<", "<<corrY<<endl;
    //cout<<"match ("<<point[0]<<", "<<point[1]<<"): "<<corrData.match<<endl;
      //cout<<"SNR ("<<point[0]<<", "<<point[1]<<") = "<<corrData.snr<<endl;
    /*if(corrData.snr > -24 && corrData.snr < -27)
    {
      return QgsComplex();
    }*/
  }
  else
  {
    //cout<<"This point could not be found in the other band"<<endl;
  }
  newPoint[0] = point[0]+corrX;
  newPoint[1] = point[1]+corrY;
  return QgsComplex(newPoint[0]-point[0], newPoint[1]-point[1]);
}

QgsRegion* QgsImageAligner::region(GDALDataset* dataset, double* point, int dimension, int band)
{
  int x = int(round(point[0]));
  int y = int(round(point[1]));
  QgsRegion *region = new QgsRegion;
  region->width = dimension;
  region->height = dimension;
  //unsigned int *data = (unsigned int*) CPLMalloc(sizeof(unsigned int)*region.width*region.height);
  uint *data = new uint[region->width*region->height];
 // mMutex->lock();
  dataset->GetRasterBand(band)->RasterIO(GF_Read, x-dimension/2, y-dimension/2, region->width, region->height, data, region->width, region->height, GDT_UInt32, 0, 0);
  //mMutex->unlock();
  for(int i = 0; i < region->width*region->height; i++)
  {
    if(data[i] > mMax)
    {
      mMax = data[i];
    }
    data[i] = mBits-data[i];
  }
  region->data = data;
  return region;
}

void QgsImageAligner::estimate(bool deleteOldDisparity)
{
  //Create empty disparity map
  mDisparityReal = (double*) malloc(mHeight*mWidth*sizeof(double));
  mDisparityImag = (double*) malloc(mHeight*mWidth*sizeof(double));
  /*mDisparityReal = new double*[mHeight];
  mDisparityImag = new double*[mHeight];
  
  for(int i = 0; i < mHeight; i++)
  {
    mDisparityReal[i] = new double[mWidth];
    mDisparityImag[i] = new double[mWidth];
  }*/

  int xSteps = ceil(mWidth / double(mBlockSize));
  int ySteps = ceil(mHeight / double(mBlockSize));
  double xDiff = (xSteps * mBlockSize - mWidth) / double(xSteps - 1);
  double yDiff = (ySteps * mBlockSize - mHeight) / double(ySteps - 1);
  int blockB = floor((mBlockSize + 1) / 2.0);
  
  int xTo = xSteps+1;
  int yTo = ySteps+1;
  for(int x = 0; x < xTo; x++)
  {
    int x1 = floor((x-0.5)*mBlockSize - fabs((double)(x-1))*xDiff);
    int x2 = floor((x+0.5)*mBlockSize - x*xDiff);
    if(!(x1 > 0 && x1 <= mWidth-blockB))
    {
      x1 = x2;
    }
    if(!(x2 > 0 && x2 <= mWidth-blockB))
    {
      x2 = x1;
    }
    
    int newX;
    int startX;
    if(x2 - x1)
    {
      newX = x2-x1;
      startX = x1;
    }
    else
    {
      newX = min(x1, mWidth-x1);
      startX = x1;
      if(x1 <= blockB)
      {
	startX = 0;
      }
    }
    for(int y = 0; y < yTo; y++)
    {
      int y1 = floor((y-0.5)*mBlockSize - fabs((double)(y-1))*yDiff);
      int y2 = floor((y+0.5)*mBlockSize - y*yDiff);
      if(!(y1 > 0 && y1 <= mHeight-blockB))
      {
	y1 = y2;
      }
      if(!(y2 > 0 && y2 <= mHeight-blockB))
      {
	y2 = y1;
      }
      
      QgsComplex p1;
      QgsComplex p2;
      QgsComplex p3;
      QgsComplex p4;
      
      if(y1 < mHeight && x1 < mWidth)
      {
	p1 = mRealDisparity[y1][x1];
      }
      else
      {
	p1 = QgsComplex(0.0, 0.0);
      }
      
      if(y1 < mHeight && x2 < mWidth)
      {
	p2 = mRealDisparity[y1][x2];
      }
      else
      {
	p2 = QgsComplex(0.0, 0.0);
      }
      
      if(y2 < mHeight && x1 < mWidth)
      {
	p3 = mRealDisparity[y2][x1];
      }
      else
      {
	p3 = QgsComplex(0.0, 0.0);
      }
      
      if(y2 < mHeight && x2 < mWidth)
      {
	p4 = mRealDisparity[y2][x2];
      }
      else
      {
	p4 = QgsComplex(0.0, 0.0);
      }

      int newY;
      int startY;
      if(y2-y1)
      {
	newY = y2-y1;
	startY = y1;
      }
      else
      {
	newY = min(y1, mHeight-y1);
	startY = y1;
	if(y1 <= blockB)
	{
	  startY = 0;
	}
      }
      	  
      for(int xSub = 0; xSub < newX; xSub++)
      {
	int newXSub = startX + xSub;
	QgsComplex top = (p2*double(xSub) + p1*double(newX-xSub)) / double(newX);
	QgsComplex bottom = (p4*double(xSub) + p3*double(newX-xSub)) / double(newX);
	for(int ySub = 0; ySub < newY; ySub++)
	{
	  int newYSub = startY + ySub;
 	  QgsComplex middle = (top*double(newY - ySub) + bottom*double(ySub)) / double(newY);
	  //cout<<"Real2: "<<newY<<" "<<y1<<" "<<y2<<" "<<p1.imag()<<" "<<middle.imag()<<endl;
	  int index = newYSub*mWidth + newXSub;
	  mDisparityReal[index] = middle.real();
	  mDisparityImag[index] = middle.imag();
	}
      }
    }
  }
  //We delete the disparity map here, to free up memory for computers with lower memory capacity
  if(deleteOldDisparity && mRealDisparity != NULL)
  {
    for(int i = 0; i < mHeight; i++)
    {
      delete [] mRealDisparity[i];
    }
    delete [] mRealDisparity;
    mRealDisparity = NULL;
  }
}

uint* QgsImageAligner::applyUInt()
{
  int size = mWidth*mHeight;
  uint *image = new uint[size];
  for(int i = 0; i < size; i++)
  {
    image[i] = 0;
  }
  
  int *base = new int[size];
  mTranslateDataset->GetRasterBand(mUnreferencedBand)->RasterIO(GF_Read, 0, 0, mWidth, mHeight, base, mWidth, mHeight, GDT_Int32, 0, 0);
  for(int x = 0; x < mWidth; x++)
  {
    for(int y = 0; y < mHeight; y++)
    {
      QgsComplex trans(mDisparityReal[y*mWidth + x], mDisparityImag[y*mWidth + x]);
      trans = trans.negative();
      double newX = floor(x+trans.real());
      double newY = floor(y+trans.imag());
      double newX2 = x+trans.real()-newX;
      double newY2 = y+trans.imag()-newY;
      double data[16];
      
      QList<int> indexes;
      indexes.append((newY-1)*mWidth + (newX-1));
      indexes.append((newY-1)*mWidth + (newX));
      indexes.append((newY-1)*mWidth + (newX+1));
      indexes.append((newY-1)*mWidth + (newX+2));
      indexes.append((newY)*mWidth + (newX-1));
      indexes.append((newY)*mWidth + (newX));
      indexes.append((newY)*mWidth + (newX+1));
      indexes.append((newY)*mWidth + (newX+2));
      indexes.append((newY+1)*mWidth + (newX-1));
      indexes.append((newY+1)*mWidth + (newX));
      indexes.append((newY+1)*mWidth + (newX+1));
      indexes.append((newY+1)*mWidth + (newX+2));
      indexes.append((newY+2)*mWidth + (newX-1));
      indexes.append((newY+2)*mWidth + (newX));
      indexes.append((newY+2)*mWidth + (newX+1));
      indexes.append((newY+2)*mWidth + (newX+2));
      
      bool broke = false;
      for(int i = 0; i < indexes.size(); i++)
      {
	if(indexes[i] < 0)
	{
	  broke = true;
	  break;
	}
	data[i] = base[indexes[i]];
      }
      if(!broke)
      {
	int value = cubicConvolution(data, QgsComplex(newX2, newY2));
	if(!(uint(value) > mBits) && !(uint(value) > mMax))
	{
	  image[((mHeight-y)*mWidth - (mWidth-x)) ] = uint(value);
	}
      }
    }
  }
  delete [] base;
  
  
  if(mDisparityReal != NULL)
  {
    free(mDisparityReal);
    mDisparityReal = NULL;
  }
  if(mDisparityImag != NULL)
  {
    free(mDisparityImag);
    mDisparityImag = NULL;
  }
  
  return image;
}

int* QgsImageAligner::applyInt()
{
  int size = mWidth*mHeight;
  int *image = new int[size];
  for(int i = 0; i < size; i++)
  {
    image[i] = 0;
  }
  
  int *base = new int[size];
  mTranslateDataset->GetRasterBand(mUnreferencedBand)->RasterIO(GF_Read, 0, 0, mWidth, mHeight, base, mWidth, mHeight, GDT_Int32, 0, 0);
  for(int x = 0; x < mWidth; x++)
  {
    for(int y = 0; y < mHeight; y++)
    {
      QgsComplex trans(mDisparityReal[y*mWidth + x], mDisparityImag[y*mWidth + x]);
      trans = trans.negative();
      double newX = floor(x+trans.real());
      double newY = floor(y+trans.imag());
      double newX2 = x+trans.real()-newX;
      double newY2 = y+trans.imag()-newY;
      double data[16];
      
      QList<int> indexes;
      indexes.append((newY-1)*mWidth + (newX-1));
      indexes.append((newY-1)*mWidth + (newX));
      indexes.append((newY-1)*mWidth + (newX+1));
      indexes.append((newY-1)*mWidth + (newX+2));
      indexes.append((newY)*mWidth + (newX-1));
      indexes.append((newY)*mWidth + (newX));
      indexes.append((newY)*mWidth + (newX+1));
      indexes.append((newY)*mWidth + (newX+2));
      indexes.append((newY+1)*mWidth + (newX-1));
      indexes.append((newY+1)*mWidth + (newX));
      indexes.append((newY+1)*mWidth + (newX+1));
      indexes.append((newY+1)*mWidth + (newX+2));
      indexes.append((newY+2)*mWidth + (newX-1));
      indexes.append((newY+2)*mWidth + (newX));
      indexes.append((newY+2)*mWidth + (newX+1));
      indexes.append((newY+2)*mWidth + (newX+2));
      
      bool broke = false;
      for(int i = 0; i < indexes.size(); i++)
      {
	if(indexes[i] < 0)
	{
	  broke = true;
	  break;
	}
	data[i] = base[indexes[i]];
      }
      if(!broke)
      {
	int value = cubicConvolution(data, QgsComplex(newX2, newY2));
	if(!(uint(value) > mBits) && !(uint(value) > mMax))
	{
	  image[((mHeight-y)*mWidth - (mWidth-x)) ] = int(value);
	}
      }
    }
  }
  delete [] base;
  
  
  if(mDisparityReal != NULL)
  {
    free(mDisparityReal);
    mDisparityReal = NULL;
  }
  if(mDisparityImag != NULL)
  {
    free(mDisparityImag);
    mDisparityImag = NULL;
  }
  
  return image;
}

double* QgsImageAligner::applyFloat()
{
  int size = mWidth*mHeight;
  double *image = new double[size];
  for(int i = 0; i < size; i++)
  {
    image[i] = 0.0;
  }
  
  int *base = new int[size];
  mTranslateDataset->GetRasterBand(mUnreferencedBand)->RasterIO(GF_Read, 0, 0, mWidth, mHeight, base, mWidth, mHeight, GDT_Int32, 0, 0);
  for(int x = 0; x < mWidth; x++)
  {
    for(int y = 0; y < mHeight; y++)
    {
      QgsComplex trans(mDisparityReal[y*mWidth + x], mDisparityImag[y*mWidth + x]);
      trans = trans.negative();
      double newX = floor(x+trans.real());
      double newY = floor(y+trans.imag());
      double newX2 = x+trans.real()-newX;
      double newY2 = y+trans.imag()-newY;
      double data[16];
      
      QList<int> indexes;
      indexes.append((newY-1)*mWidth + (newX-1));
      indexes.append((newY-1)*mWidth + (newX));
      indexes.append((newY-1)*mWidth + (newX+1));
      indexes.append((newY-1)*mWidth + (newX+2));
      indexes.append((newY)*mWidth + (newX-1));
      indexes.append((newY)*mWidth + (newX));
      indexes.append((newY)*mWidth + (newX+1));
      indexes.append((newY)*mWidth + (newX+2));
      indexes.append((newY+1)*mWidth + (newX-1));
      indexes.append((newY+1)*mWidth + (newX));
      indexes.append((newY+1)*mWidth + (newX+1));
      indexes.append((newY+1)*mWidth + (newX+2));
      indexes.append((newY+2)*mWidth + (newX-1));
      indexes.append((newY+2)*mWidth + (newX));
      indexes.append((newY+2)*mWidth + (newX+1));
      indexes.append((newY+2)*mWidth + (newX+2));
      
      bool broke = false;
      for(int i = 0; i < indexes.size(); i++)
      {
	if(indexes[i] < 0)
	{
	  broke = true;
	  break;
	}
	data[i] = base[indexes[i]];
      }
      if(!broke)
      {
	int value = cubicConvolution(data, QgsComplex(newX2, newY2));
	if(!(uint(value) > mBits) && !(uint(value) > mMax))
	{
	  image[((mHeight-y)*mWidth - (mWidth-x)) ] = double(value);
	}
      }
    }
  }
  delete [] base;
  
  
  if(mDisparityReal != NULL)
  {
    free(mDisparityReal);
    mDisparityReal = NULL;
  }
  if(mDisparityImag != NULL)
  {
    free(mDisparityImag);
    mDisparityImag = NULL;
  }
  
  return image;
}

int QgsImageAligner::cubicConvolution(double *array, QgsComplex point)
{
  double xShift = point.real();
  double yShift = point.imag();
  
  double *xArray = new double[4];
  double *yArray = new double[4];
  xArray[0] = -(1/2.0)*pow(xShift, 3) + pow(xShift, 2) - (1/2.0)*xShift;
  xArray[1] = (3/2.0)*pow(xShift, 3) - (5/2.0)*pow(xShift, 2) + 1;
  xArray[2] = -(3/2.0)*pow(xShift, 3) + 2*pow(xShift, 2) + (1/2.0)*xShift;
  xArray[3] = (1/2.0)*pow(xShift, 3) - (1/2.0)*pow(xShift, 2);
  
  yArray[0] = -(1/2.0)*pow(yShift, 3) + pow(yShift, 2) - (1/2.0)*yShift;
  yArray[1] = (3/2.0)*pow(yShift, 3) - (5/2.0)*pow(yShift, 2) + 1;
  yArray[2] = -(3/2.0)*pow(yShift, 3) + 2*pow(yShift, 2) + (1/2.0)*yShift;
  yArray[3] = (1/2.0)*pow(yShift, 3) - (1/2.0)*pow(yShift, 2);
  
  int result = 0;
  for(int y = 0; y < 4; y++)
  {
    int rowContribution = 0;
    for(int x = 0; x < 4; x++)
    {
      rowContribution += xArray[x] * array[y*4 + x];
    }
    result += rowContribution * yArray[y];
  }
  
  delete [] xArray;
  delete [] yArray;
  
  return result;
}

QgsPointDetectionThread::QgsPointDetectionThread(QMutex *mutex, GDALDataset *inputDataset, GDALDataset *translateDataset, int referenceBand, int unreferencedBand, int blockSize, int bits, int *max)
{
  mMutex = mutex;
  mInputDataset = inputDataset;
  mTranslateDataset = translateDataset;
  mReferenceBand = referenceBand;
  mUnreferencedBand = unreferencedBand;
  mBlockSize = blockSize;
  mBits = bits;
  mMax = max;
  mPoint = new double[2];
}

void QgsPointDetectionThread::setPoint(double *point, QgsComplex estimate)
{
  mPoint[0] = point[0];
  mPoint[1] = point[1];
  mEstimate = estimate;
}

void QgsPointDetectionThread::run()
{
  double newPoint[2];
  newPoint[0] = mPoint[0] + mEstimate.real();
  newPoint[1] = mPoint[1] + mEstimate.imag();
  double corrX = mEstimate.real();
  double corrY = mEstimate.imag();
  QgsRegion *region1 = region(mInputDataset, mPoint, mBlockSize, mReferenceBand);
  QgsRegion *region2 = region(mTranslateDataset, newPoint, mBlockSize, mUnreferencedBand);
  
  QgsRegionCorrelationResult corrData = QgsRegionCorrelator::findRegion(region1, region2, mBits);
  delete [] region1->data;
  delete [] region2->data;
  delete region1;
  delete region2;
  if(corrData.wasSet)
  {
    corrX = corrData.x;
    corrY = corrData.y;
    //cout<<"Point found: "<<corrX<<", "<<corrY<<endl;
  }
  else
  {
   // cout<<"This point could not be found in the other band"<<endl;
  }
  newPoint[0] = mPoint[0]+corrX;
  newPoint[1] = mPoint[1]+corrY;
  mResult = QgsComplex(newPoint[0]-mPoint[0], newPoint[1]-mPoint[1]);
}

QgsRegion* QgsPointDetectionThread::region(GDALDataset* dataset, double* point, int dimension, int band)
{
  int x = int(round(point[0]));
  int y = int(round(point[1]));
  QgsRegion *region = new QgsRegion;
  region->width = dimension;
  region->height = dimension;
  //unsigned int *data = (unsigned int*) CPLMalloc(sizeof(unsigned int)*region.width*region.height);
  uint *data = new uint[region->width*region->height];
  mMutex->lock();
  dataset->GetRasterBand(band)->RasterIO(GF_Read, x-dimension/2, y-dimension/2, region->width, region->height, data, region->width, region->height, GDT_UInt32, 0, 0);
  mMutex->unlock();
  for(int i = 0; i < region->width*region->height; i++)
  {
    if(data[i] > *mMax)
    {
      *mMax = data[i];
    }
    data[i] = mBits-data[i];
  }
  region->data = data;
  return region;
}