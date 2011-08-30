#include "qgsbandaligner.h"

QgsBandAligner::QgsBandAligner(QStringList input, QString output, QString disparityXPath, QString disparityYPath, int blockSize, int referenceBand, QgsBandAligner::Transformation transformation, int minimumGcps, double refinementTolerance)
{
  mThread = new QgsBandAlignerThread(input, output, disparityXPath, disparityYPath, blockSize, referenceBand, transformation, minimumGcps, refinementTolerance);
  QObject::connect(mThread, SIGNAL(progressed(double)), this, SLOT(progressReceived(double)));
  QObject::connect(mThread, SIGNAL(logged(QString)), this, SLOT(logReceived(QString)));
}

QgsBandAligner::~QgsBandAligner()
{
  if(mThread != NULL)
  {
    delete mThread;
    mThread = NULL;
  }
}

void QgsBandAligner::start()
{
  mThread->start();
}

void QgsBandAligner::stop()
{
  mThread->stop();
}
    
void QgsBandAligner::logReceived(QString message)
{
  emit logged(message);
}

void QgsBandAligner::progressReceived(double progress)
{
  emit progressed(progress);
}
    
    
    
QgsBandAlignerThread::QgsBandAlignerThread(QStringList input, QString output, QString disparityXPath, QString disparityYPath, int blockSize, int referenceBand, QgsBandAligner::Transformation transformation, int minimumGcps, double refinementTolerance)
  : QThread()
{
  mReferenceBand = /*referenceBand*/1;
  mInputPath = input;
  mOutputPath = output;
  mDisparityXPath = disparityXPath;
  mDisparityYPath = disparityYPath;
  mBlockSize = blockSize;
  mInputDataset = NULL;
  mOutputDataset = NULL;
  mBands = 0;
  mWidth = 0;
  mHeight = 0;
  mStopped = false;
  mTransformation = transformation;
  mMinimumGcps = minimumGcps;
  mRefinementTolerance = refinementTolerance;
}

QgsBandAlignerThread::~QgsBandAlignerThread()
{
  if(mInputDataset != NULL)
  {
    GDALClose(mInputDataset);
    mInputDataset = NULL;
  }
  if(mOutputDataset != NULL)
  {
    GDALClose(mOutputDataset);
    mOutputDataset = NULL;
  }
}

void QgsBandAlignerThread::stop()
{
  mStopped = true;
}

void QgsBandAlignerThread::run()
{
  double progress = 0;
  emit progressed(progress);
  emit logged("Preparing input data ...");
  if (GDALGetDriverCount() == 0)
  {
    GDALAllRegister();
  }
  mInputDataset = (GDALDataset*) GDALOpen(mInputPath[mReferenceBand].toAscii().data(), GA_ReadOnly);
  mBands = mInputPath.size();
  mWidth = mInputDataset->GetRasterXSize();
  mHeight = mInputDataset->GetRasterYSize();
  if(mInputPath.size() == 1)
  {
    mBands = mInputDataset->GetRasterCount();
  }
  
  progress += 1;
  emit progressed(progress);
  
  if(mStopped)
  {
    emit progressed(100);
    emit logged("Process determinated.");
    return;
  }
  
  GDALDataType type = mInputDataset->GetRasterBand(1)->GetRasterDataType();
  
  progress += 1;
  emit progressed(progress);
  
  if(mTransformation == QgsBandAligner::Disparity)
  {
    mOutputDataset = mInputDataset->GetDriver()->Create(mOutputPath.toAscii().data(), mWidth, mHeight, mBands, type, 0);
    
    char **metadata = mInputDataset->GetDriver()->GetMetadata();
    
    //Flip write the first band
    int row = 0;
    for(int i = mHeight-1; i >= 0; i--)
    {
      int *firstBand = (int*) malloc(sizeof(int) * mWidth);
      mInputDataset->GetRasterBand(1)->RasterIO(GF_Read, 0, row, mWidth, 1, firstBand, mWidth, 1, type, 0, 0);
      mOutputDataset->GetRasterBand(1)->RasterIO(GF_Write, 0, i, mWidth, 1, firstBand, mWidth, 1, type, 0, 0);
      row++;
      free(firstBand);
    }
    
    mOutputDataset->SetMetadata(metadata);
    mOutputDataset->FlushCache();
  }
  GDALDataset *disparityMapX = NULL;
  GDALDataset *disparityMapY = NULL;
  
  if(mStopped)
  {
    emit progressed(100);
    emit logged("Process determinated.");
    return;
  }
  
  progress += 2;
  emit progressed(progress);

  if(mDisparityXPath != "" && mTransformation == QgsBandAligner::Disparity)
  {
    disparityMapX = GetGDALDriverManager()->GetDriverByName("GTiff")->Create(mDisparityXPath.toAscii().data(), mWidth, mHeight, mBands, GDT_Float32, NULL);
  }
  if(mDisparityYPath != "" && mTransformation == QgsBandAligner::Disparity)
  {
    disparityMapY = GetGDALDriverManager()->GetDriverByName("GTiff")->Create(mDisparityYPath.toAscii().data(), mWidth, mHeight, mBands, GDT_Float32, NULL);
  }
  
  if(mTransformation != QgsBandAligner::Disparity)
  {
    QString gcps = "-gcp 0 0 0 0 -gcp 0 "+QString::number(mHeight)+" 0 "+QString::number(mHeight)+" -gcp "+QString::number(mWidth)+" 0 "+QString::number(mWidth)+" 0 -gcp "+QString::number(mWidth)+" "+QString::number(mHeight)+" "+QString::number(mWidth)+" "+QString::number(mHeight)+" ";
    QProcess p;
    p.start("gdal_translate -a_srs EPSG:4326 "+gcps+mInputPath[mReferenceBand-1]+" "+mInputPath[mReferenceBand-1]+".gcps");
    p.waitForFinished(999999);
    p.start("gdalwarp -multi -t_srs EPSG:4326 "+mInputPath[mReferenceBand-1]+".gcps "+mInputPath[mReferenceBand-1]+".warped");
    p.waitForFinished(999999);
    QFile f(mInputPath[mReferenceBand-1]+".gcps");
    f.remove();
  }
  
  QList<int> bands;
  for(int i = 0; i < mBands; i++)
  {
    if(mReferenceBand != (i+1))
    {
      bands.append(i+1);
    }
  }
  
  if(mStopped)
  {
    emit progressed(100);
    emit logged("Process determinated.");
    return;
  }
  
  progress += 1;
  emit progressed(progress);
 
  double div = bands.size();
  for(int i = 0; i < bands.size(); i++)
  {
    emit logged("Starting to analyze band "+QString::number(bands[i])+" ...");
    QgsImageAligner *aligner;
    if(mInputPath.size() == 1)
    {
      aligner = new QgsImageAligner(mInputPath[0], mInputPath[0], mReferenceBand, bands[i], mBlockSize);
    }
    else
    {
      aligner = new QgsImageAligner(mInputPath[mReferenceBand-1], mInputPath[bands[i]-1], 1, 1, mBlockSize);
    }
    
    if(mStopped)
    {
      emit progressed(100);
      emit logged("Process determinated.");
      return;
    }
    
    progress += 1/div;
    emit progressed(progress);
    
    emit logged("Scanning band "+QString::number(bands[i])+" ...");
    aligner->scan();
    progress += 24/div;
    emit progressed(progress);
    if(mStopped)
    {
      emit progressed(100);
      emit logged("Process determinated.");
      return;
    }
    
    if(mTransformation == QgsBandAligner::Disparity)
    {
      aligner->eliminate(mRefinementTolerance);
      emit logged("Estimating pixels for band "+QString::number(bands[i])+" ...");
      aligner->estimate();
      
      progress += 20/div;
      emit progressed(progress);
      if(mStopped)
      {
	emit progressed(100);
	emit logged("Process determinated.");
	return;
      }

      if(disparityMapX != NULL)
      {
	emit logged("Writing the x-based disparity map for band "+QString::number(bands[i])+" ...");
	disparityMapX->GetRasterBand(bands[i])->RasterIO(GF_Write, 0, 0, aligner->width(), aligner->height(), aligner->disparityReal(), aligner->width(), aligner->height(), GDT_Float64, 0, 0);
	disparityMapX->FlushCache();
      }
      
      progress += 10/div;
      emit progressed(progress);
      if(mStopped)
      {
	emit progressed(100);
	emit logged("Process determinated.");
	return;
      }
      
      if(disparityMapY != NULL)
      {
	emit logged("Writing the y-based disparity map for band "+QString::number(bands[i])+" ...");
	disparityMapY->GetRasterBand(bands[i])->RasterIO(GF_Write, 0, 0, aligner->width(), aligner->height(), aligner->disparityImag(), aligner->width(), aligner->height(), GDT_Float64, 0, 0);
	disparityMapY->FlushCache();
      }
      
      progress += 10/div;
      emit progressed(progress);
      if(mStopped)
      {
	emit progressed(100);
	emit logged("Process determinated.");
	return;
      }
      
      if(type == GDT_UInt32 || type == GDT_UInt16)
      {
	
	uint *data = aligner->applyUInt();
	mOutputDataset->GetRasterBand(bands[i])->RasterIO(GF_Write, 0, 0, aligner->width(), aligner->height(), data, aligner->width(), aligner->height(), GDT_UInt32, 0, 0);
	mOutputDataset->FlushCache();
	delete [] data;
      }
      else if(type == GDT_Float32)
      {
	double *data = aligner->applyFloat();
	mOutputDataset->GetRasterBand(bands[i])->RasterIO(GF_Write, 0, 0, aligner->width(), aligner->height(), data, aligner->width(), aligner->height(), GDT_Float32, 0, 0);
	mOutputDataset->FlushCache();
	delete [] data;
      }
      else
      {
	int *data = aligner->applyInt();
	mOutputDataset->GetRasterBand(bands[i])->RasterIO(GF_Write, 0, 0, aligner->width(), aligner->height(), data, aligner->width(), aligner->height(), GDT_Int32, 0, 0);
	mOutputDataset->FlushCache();
	delete [] data;
      }
    }
    else
    {
      QProcess p;
      QString gcps = "";
      double xSteps = ceil(mWidth / double(mBlockSize));
      double ySteps = ceil(mHeight / double(mBlockSize));
      for(int h = 0; h < ySteps; h++)
      {
	for(int w = 0; w < xSteps; w++)
	{
	  if(aligner->gcps()[h][w].real() == aligner->gcps()[h][w].real() && aligner->gcps()[h][w].imag() == aligner->gcps()[h][w].imag())
	  {
	    double xDis = aligner->gcps()[h][w].real();
	    double yDis = aligner->gcps()[h][w].imag();
	    gcps += " -gcp " + QString::number(w*mBlockSize) + " " + QString::number(h*mBlockSize) + " " + QString::number(w*mBlockSize+xDis) + " " + QString::number(h*mBlockSize+yDis);
	  }
	}
      }
      emit logged("Processing the GCPs for band "+QString::number(bands[i])+".");
      progress += 20/div;
      emit progressed(progress);
      if(mStopped)
      {
	emit progressed(100);
	emit logged("Process determinated.");
	return;
      }
      emit logged("Adding the GCPs to band "+QString::number(bands[i])+".");
      p.start("gdal_translate -a_srs EPSG:4326 "+gcps+" "+mInputPath[bands[i]-1]+" "+mInputPath[bands[i]-1]+".gcps");
      p.waitForFinished(999999);
      progress += 20/div;
      emit progressed(progress);
      if(mStopped)
      {
	emit progressed(100);
	emit logged("Process determinated.");
	return;
      }
      emit logged("Warping band "+QString::number(bands[i])+".");
      if(mTransformation == QgsBandAligner::ThinPlateSpline)
      {
	p.start("gdalwarp -multi -t_srs EPSG:4326 -tps "+mInputPath[bands[i]-1]+".gcps "+mInputPath[bands[i]-1]+".warped");
	p.waitForFinished(999999);
      }
      else if(mTransformation == QgsBandAligner::Polynomial2)
      {
	p.start("gdalwarp -multi -t_srs EPSG:4326 -order 2 -refine_gcps "+QString::number(mRefinementTolerance)+" "+QString::number(mMinimumGcps)+" "+mInputPath[bands[i]-1]+".gcps "+mInputPath[bands[i]-1]+".warped");
	p.waitForFinished(999999);
      }
      else if(mTransformation == QgsBandAligner::Polynomial3)
      {
	p.start("gdalwarp -multi -t_srs EPSG:4326 -order 3 -refine_gcps "+QString::number(mRefinementTolerance)+" "+QString::number(mMinimumGcps)+" "+mInputPath[bands[i]-1]+".gcps "+mInputPath[bands[i]-1]+".warped");
	p.waitForFinished(999999);
      }
      else if(mTransformation == QgsBandAligner::Polynomial4)
      {
	p.start("gdalwarp -multi -t_srs EPSG:4326 -order 4 -refine_gcps "+QString::number(mRefinementTolerance)+" "+QString::number(mMinimumGcps)+" "+mInputPath[bands[i]-1]+".gcps "+mInputPath[bands[i]-1]+".warped");
	p.waitForFinished(999999);
      }
      else if(mTransformation == QgsBandAligner::Polynomial5)
      {
	p.start("gdalwarp -multi -t_srs EPSG:4326 -order 5 -refine_gcps "+QString::number(mRefinementTolerance)+" "+QString::number(mMinimumGcps)+" "+mInputPath[bands[i]-1]+".gcps "+mInputPath[bands[i]-1]+".warped");
	p.waitForFinished(999999);
      }
      QFile f(mInputPath[bands[i]-1]+".gcps");
      f.remove();
    }
    progress += 29/div;
    emit progressed(progress);
    delete aligner;
    emit logged("Alignment for band "+QString::number(bands[i])+" completed.");
  }
  
  emit logged("Finalizing image structure and metadata...");
  if(mTransformation != QgsBandAligner::Disparity)
  {
    QProcess p;
    QString images = "";
    for(int i = 0; i < mInputPath.size(); i++)
    {
      images += mInputPath[i]+".warped ";
    }

    p.start("gdal_merge.py -separate "+images+"-o "+mOutputPath.toAscii().data()+".geoless");
    p.waitForFinished(99999999);
    
    QString gcps = "";
    int gcpCount = mInputDataset->GetGCPCount();
    if(gcpCount == 4)
    {
      gcps += "-gcp 0.5 0.5 "+QString::number(mInputDataset->GetGCPs()[3].dfGCPX)+" "+QString::number(mInputDataset->GetGCPs()[3].dfGCPY)+" ";
      gcps += "-gcp 8414.5 0.5 "+QString::number(mInputDataset->GetGCPs()[2].dfGCPX)+" "+QString::number(mInputDataset->GetGCPs()[2].dfGCPY)+" ";
      gcps += "-gcp 8414.5 9587.5 "+QString::number(mInputDataset->GetGCPs()[1].dfGCPX)+" "+QString::number(mInputDataset->GetGCPs()[1].dfGCPY)+" ";
      gcps += "-gcp 0.5 9587.5 "+QString::number(mInputDataset->GetGCPs()[0].dfGCPX)+" "+QString::number(mInputDataset->GetGCPs()[0].dfGCPY)+" ";
    }
    
    p.start("gdal_translate "+gcps+mOutputPath.toAscii().data()+".geoless "+mOutputPath.toAscii().data());
    p.waitForFinished(99999999);
    
    for(int i = 0; i < mInputPath.size(); i++)
    {
      QFile warped(mInputPath[i]+".warped");
      warped.remove();
    }
    QFile geo(mOutputPath.toAscii().data()+QString(".geoless"));
    geo.remove();
  }
  

  if(mOutputDataset != NULL)
  {
    GDALClose(mOutputDataset);
    mOutputDataset = NULL;
  }
  if(mInputDataset != NULL)
  {
    GDALClose(mInputDataset);
    mInputDataset = NULL;
  }
  if(disparityMapX != NULL)
  {
    GDALClose(disparityMapX);
  }
  if(disparityMapY != NULL)
  {
    GDALClose(disparityMapY);
  }
  progress = 100.0;
  emit progressed(progress);
  emit logged("Band alignment for the image completed.");
  emit logged("<<Finished>>");
}