#include "qgsbandaligner.h"
#include "qgsprogressmonitor.h"

#include <QDir>
#define QGISDEBUG
#include <QDebug>
#include <QFuture>
#include <qgslogger.h>
#include <QtCore>

void QgsBandAligner::start()
{
    printf("QgsBandAligner::start");
    Q_CHECK_PTR(mMonitor);
    QFuture<void> future = QtConcurrent::run(QgsBandAligner::execute, mMonitor, this);  
}

void QgsBandAligner::stop()
{
    printf("QgsBandAligner::stop");

    if (mMonitor != NULL)
        mMonitor->Cancel();
}

void QgsBandAligner::execute(QgsProgressMonitor *monitor, QgsBandAligner *self)
{
    Q_CHECK_PTR(self);    
    Q_CHECK_PTR(monitor);
    try 
    {
        switch (self->mTransformationType) 
        {
        case QgsBandAligner::Disparity:
            self->executeDisparity(*monitor);
            break;

        default:
            self->executeWarp(*monitor);
            break;
        }
    } 
    catch(...)    
    {
        monitor->Log("Caught exception!"); 
    }
    if (!monitor->IsCanceled())
         monitor->Log("<<Finished>>");
    monitor->Finish();
}

void QgsBandAligner::executeDisparity(QgsProgressMonitor &monitor)
{
    monitor.Log("Preparing input data ...");

    if (GDALGetDriverCount() == 0)
        GDALAllRegister();  

    if (monitor.IsCanceled()) 
        return;

    // Build a list of datasets from the input files
    QList<GDALDataset*> inputDatasetList;
    for (int i = 0; i < mInputPaths.size(); ++i) 
    {
        GDALDataset *dataset = (GDALDataset*) GDALOpen(mInputPaths[i].toAscii().data(), GA_ReadOnly);

        if (dataset == NULL) {
            monitor.Log("ERROR: Unable to open file: " + mInputPaths[i]);
            monitor.Log("Aborting ...");
            for (int i = 0; i < inputDatasetList.size(); ++i)
                GDALClose(inputDatasetList[i]);
            return;
        }

        Q_CHECK_PTR(dataset);
        inputDatasetList.append(dataset);
    }    

    // There must be at least 1 dataset in the list
    if (inputDatasetList.size() < 1)  {
        monitor.Log("ERROR: Not enough input files to perform band alignment.");
        monitor.Log("Aborting ..."); 
        // No need to close datasets, because the list is empty.
        return;
    }
    
    // Build a list of raster bands from the input datasets
    QList<GDALRasterBand*> inputBands;    
    if (inputDatasetList.size() == 1)
    {
        for (int i = 1; i <= inputDatasetList[0]->GetRasterCount(); ++i) {
            inputBands.append(inputDatasetList[0]->GetRasterBand(i));
        }
    }
    else
    {
        for (int i = 0; i < inputDatasetList.size(); ++i) {
            inputBands.append(inputDatasetList[i]->GetRasterBand(1));
        }
    }

    // There must be at least 2 raster bands in the list
    if (inputBands.size() < 2) {
        monitor.Log("ERROR: Not enough input bands to perform band alignment.");
        monitor.Log("Aborting ...");
        for (int i = 0; i < inputDatasetList.size(); ++i)
            GDALClose(inputDatasetList[i]);
        return;
    }
    if (!(1 <= mReferenceBand && mReferenceBand <= inputBands.size())) {
        // TODO: error message
        monitor.Log("WARNING: Reference band is out of range. Using first band in the list.");
        mReferenceBand = 1; // default to the first band
    }
    // Input value is one-based, so we subtract 1 to make it zero-based.
    mReferenceBand -= 1; 

    Q_ASSERT(inputBands.size() > 1);
    Q_ASSERT(mReferenceBand < inputBands.size());    

    QString disparityPath;

    if (monitor.IsCanceled()) 
        goto exit1;
    
    monitor.Log("Performing band alignment on the image...");

    /*QgsImageAligner::*/performImageAlignment(monitor, 
        mOutputPath, mDisparityXPath, mDisparityYPath, 
        disparityPath, inputBands, mReferenceBand, mBlockSize);
  
    monitor.Log("Band alignment for the image completed.");    
    if (!monitor.IsCanceled()) {
         monitor.SetProgress(100.0);
    }
exit1:
    if (monitor.IsCanceled()) {
        monitor.Log("Process determinated.");
    }
    for (int i = 0; i < inputDatasetList.size(); ++i) {
        GDALClose(inputDatasetList[i]);
    }
}
    
void QgsBandAligner::executeWarp(QgsProgressMonitor &monitor)
{
}

void QgsBandAligner::logReceived(QString message)
{
  emit logged(message);
}

void QgsBandAligner::progressReceived(double progress)
{
  emit progressed(progress);
}
    
QgsBandAlignerThread::QgsBandAlignerThread(
    QStringList input, QString output, 
    QString disparityXPath, QString disparityYPath, 
    int blockSize, int referenceBand, 
    QgsBandAligner::Transformation transformation, 
    int minimumGcps, double refinementTolerance)
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
    QgsProgressMonitor mMonitor;

    if (mTransformation == QgsBandAligner::Disparity) 
    {
        run_disparity(mMonitor);
    }
    else
    {
        run_gdalwarp();
    }
}

void QgsBandAlignerThread::run_disparity(QgsProgressMonitor &monitor)
{
#if 0
    if (mInputDataset != NULL)
        GDALClose(mInputDataset);

    mInputDataset = (GDALDataset*) GDALOpen(mInputPath[mReferenceBand].toAscii().data(), GA_ReadOnly);
    mBands = mInputPath.size();
    mWidth = mInputDataset->GetRasterXSize();
    mHeight = mInputDataset->GetRasterYSize();
    if(mInputPath.size() == 1)
    {
        mBands = mInputDataset->GetRasterCount();
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
        goto out1;
    } else {
        progress += 1;
        emit progressed(progress);
    }
  
    GDALDataType type = mInputDataset->GetRasterBand(1)->GetRasterDataType();
  
    progress += 1;
    emit progressed(progress);
  
#if 0
    if(mTransformation == QgsBandAligner::Disparity)
    {
        mOutputDataset = mInputDataset->GetDriver()->Create(mOutputPath.toAscii().data(), mWidth, mHeight, mBands, type, 0);

        char **metadata = mInputDataset->GetDriver()->GetMetadata();

        QgsDebugMsg("Flipping first band vertically");

        //Flip write the first band
        int *scanlineBuffer = (int*) malloc(sizeof(int) * mWidth);
        for(int i = mHeight - 1, row = 0; i >= 0; row++, i--)
        {
            mInputDataset->GetRasterBand(1)->RasterIO(GF_Read, 0, row, mWidth, 1, scanlineBuffer, mWidth, 1, type, 0, 0);
            mOutputDataset->GetRasterBand(1)->RasterIO(GF_Write, 0, i, mWidth, 1, scanlineBuffer, mWidth, 1, type, 0, 0);
        }
        free(scanlineBuffer);

        mOutputDataset->SetMetadata(metadata);
        mOutputDataset->FlushCache();
    } 
    else 
    {
        mOutputDataset = NULL;
    }
#endif
  
    if(mStopped)
    {
        emit progressed(100);
        emit logged("Process determinated.");
        goto out2;
    }
    else 
    {  
        progress += 2;
        emit progressed(progress);
    }

    if (mDisparityXPath == "") {
        mDisparityXPath = QDir::tempPath() + "/disparity-map-x.tif";
    }
    GDALDataset *disparityMapX = GetGDALDriverManager()->GetDriverByName("GTiff")->Create(mDisparityXPath.toAscii().data(), mWidth, mHeight, mBands, GDT_Float32, NULL);

    if (mDisparityYPath == "") {
        mDisparityYPath = QDir::tempPath() + "/disparity-map-y.tif";
    }      
    GDALDataset *disparityMapY = GetGDALDriverManager()->GetDriverByName("GTiff")->Create(mDisparityYPath.toAscii().data(), mWidth, mHeight, mBands, GDT_Float32, NULL);
  
  if(mStopped)
  {
    emit progressed(100);
    emit logged("Process determinated.");
    goto out3;
  }
  else
  {
    progress += 1;
    emit progressed(progress);
  }

  double div = bands.size();
  for(int i = 0; i < bands.size(); i++)
  {
    emit logged("Starting to analyze band "+QString::number(bands[i])+" ...");

    QgsImageAligner *aligner = NULL;
    
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
        delete aligner;      
        emit progressed(100);
        emit logged("Process determinated.");
        goto out3;
    }
    else
    {
        progress += 1/div;
        emit progressed(progress);
    }
    
    emit logged("Scanning band "+QString::number(bands[i])+" ...");
    
    // ************************** STEP 1 ********************
    aligner->scan(); 
    
    if(mStopped)
    {
        delete aligner;
        emit progressed(100);
        emit logged("Process determinated.");
        goto out3;
    }
    else
    {
        progress += 24/div;
        emit progressed(progress);
    }

    //-------------------------------------------------------------------------   

    // ************************** STEP 2 ********************
    aligner->eliminate(mRefinementTolerance);   

    emit logged("Estimating pixels for band "+QString::number(bands[i])+" ...");

    // ************************** STEP 3 ********************
    aligner->estimate(); 
      
    if(mStopped)
    {
        delete aligner;
        emit progressed(100);
        emit logged("Process determinated.");
        goto out3;
    }
    else
    {
        progress += 20/div;
        emit progressed(progress);
    }

    if(disparityMapX != NULL)
    {
        emit logged("Writing the x-based disparity map for band "+QString::number(bands[i])+" ...");
        disparityMapX->GetRasterBand(bands[i])->RasterIO(GF_Write, 0, 0, aligner->width(), aligner->height(), aligner->disparityReal(), aligner->width(), aligner->height(), GDT_Float64, 0, 0);
        disparityMapX->FlushCache();
    }

    if(mStopped)
    {
        delete aligner;
        emit progressed(100);
        emit logged("Process determinated.");
        goto out3;
    } 
    else 
    {  
        progress += 10/div;
        emit progressed(progress);
    }

    if(disparityMapY != NULL)
    {
        emit logged("Writing the y-based disparity map for band "+QString::number(bands[i])+" ...");
        disparityMapY->GetRasterBand(bands[i])->RasterIO(GF_Write, 0, 0, aligner->width(), aligner->height(), aligner->disparityImag(), aligner->width(), aligner->height(), GDT_Float64, 0, 0);
        disparityMapY->FlushCache();
    }

    if(mStopped)
    {
        delete aligner;
        emit progressed(100);
        emit logged("Process determinated.");
        goto out3;
    }
    else
    {
        progress += 10/div;
        emit progressed(progress);
    }

    aligner->alignImageBand(disparityMapX, disparityMapY, mOutputDataset, bands[i]);
    mOutputDataset->FlushCache();

#if 1
    QgsDebugMsg("[OUTPUT] pixel type = " + (int)type);
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
#endif
    //-------------------------------------------------------------------------

    progress += 29/div;
    emit progressed(progress);

    delete aligner;

    emit logged("Alignment for band "+QString::number(bands[i])+" completed.");
  } 

  progress = 100.0;
  emit progressed(progress);
  emit logged("Band alignment for the image completed.");
  emit logged("<<Finished>>");

out3:
  if(disparityMapX != NULL)
  {
    GDALClose(disparityMapX);
  }
  if(disparityMapY != NULL)
  {
    GDALClose(disparityMapY);
  }

out2:
  if(mOutputDataset != NULL)
  {
    GDALClose(mOutputDataset);
    mOutputDataset = NULL;
  }

out1:
  if(mInputDataset != NULL)
  {
    GDALClose(mInputDataset);
    mInputDataset = NULL;
  }
#endif
}

/* ************************************************************************* */

void QgsBandAlignerThread::run_gdalwarp()
{
    double progress = 0;
    emit progressed(progress);
    emit logged("Preparing input data ...");  

    Q_ASSERT(mTransformation != QgsBandAligner::Disparity);

    if (GDALGetDriverCount() == 0)
        GDALAllRegister();  

    if (mInputDataset != NULL)
        GDALClose(mInputDataset);

    mBands = mInputPath.size();

    mInputDataset = (GDALDataset*) GDALOpen(mInputPath[mReferenceBand].toAscii().data(), GA_ReadOnly);    
    mWidth = mInputDataset->GetRasterXSize();
    mHeight = mInputDataset->GetRasterYSize();
    
    if(mInputPath.size() == 1)
    {
        mBands = mInputDataset->GetRasterCount();
    }

    QList<int> bands;
    for(int i = 0; i < mBands; i++)
    {
        if(mReferenceBand != (i+1))
        {
            bands.append(i+1);
        }
    }

    if(mStopped) goto exit;
    progress += 4;
    emit progressed(progress);

    //-------------------------------------------------------
    {
        QString gcps = "-gcp 0 0 0 0 -gcp 0 "+QString::number(mHeight)+" 0 "+QString::number(mHeight)+" -gcp "+QString::number(mWidth)+" 0 "+QString::number(mWidth)+" 0 -gcp "+QString::number(mWidth)+" "+QString::number(mHeight)+" "+QString::number(mWidth)+" "+QString::number(mHeight)+" ";

        QString cmdline = "gdal_translate -a_srs EPSG:4326 "+gcps+mInputPath[mReferenceBand-1]+" "+mInputPath[mReferenceBand-1]+".gcps";

        QProcess p;
        QgsDebugMsg("Calling: '" + cmdline + "'");
        p.start(cmdline);
        p.waitForFinished(999999);
        
        cmdline = "gdalwarp -multi -t_srs EPSG:4326 "+mInputPath[mReferenceBand-1]+".gcps "+mInputPath[mReferenceBand-1]+".warped";
        QgsDebugMsg("Calling: '" + cmdline + "'");
        p.start(cmdline);
        p.waitForFinished(999999);
        
        QFile f(mInputPath[mReferenceBand-1]+".gcps");
        QgsDebugMsg("Removing file: '" + f.fileName() + "'");
        f.remove();
    }

    if(mStopped) goto exit;
    progress += 1;
    emit progressed(progress);

    //-------------------------------------------------------

    QgsImageAligner *aligner = NULL;    
    double div = bands.size();

    for(int i = 0; i < bands.size(); i++)
    {
        emit logged("Starting to analyze band "+QString::number(bands[i])+" ...");

        if(mInputPath.size() == 1)
        {
            aligner = new QgsImageAligner(mInputPath[0], mInputPath[0], mReferenceBand, bands[i], mBlockSize);
        }
        else
        {
            aligner = new QgsImageAligner(mInputPath[mReferenceBand-1], mInputPath[bands[i]-1], 1, 1, mBlockSize);
        }

        if(mStopped) break;
        progress += 1/div;
        emit progressed(progress);

        //-------------------------------------------------------------------------

        emit logged("Scanning band " + QString::number(bands[i]) + " ...");

//        aligner->scan();  // FIXME

        if(mStopped) break;
        progress += 24/div;
        emit progressed(progress);

        //-------------------------------------------------------------------------
// FIXME: need to be rewritten to use the new disparity file 
        QString gcps = "";
        double xSteps = ceil(mWidth / double(mBlockSize));
        double ySteps = ceil(mHeight / double(mBlockSize));
        for(int h = 0; h < ySteps; h++)
        {            
            for(int w = 0; w < xSteps; w++)
            {
#if 0 
                if (aligner->gcps()[h][w].real() == aligner->gcps()[h][w].real() && 
                    aligner->gcps()[h][w].imag() == aligner->gcps()[h][w].imag())
                {
                    double xDis = aligner->gcps()[h][w].real();
                    double yDis = aligner->gcps()[h][w].imag();
                    gcps += " -gcp " + QString::number(w*mBlockSize) + " " + QString::number(h*mBlockSize);
                    gcps += " " + QString::number(w*mBlockSize+xDis) + " " + QString::number(h*mBlockSize+yDis);
                }
#endif
            }
        }

        //-------------------------------------------------------------------------

        emit logged("Processing the GCPs for band " + QString::number(bands[i]) + ".");

        if (mStopped) break;
        progress += 20/div;
        emit progressed(progress);

        emit logged("Adding the GCPs to band "+QString::number(bands[i])+".");
        QProcess p;
        p.start("gdal_translate -a_srs EPSG:4326 "+gcps+" "+mInputPath[bands[i]-1]+" "+mInputPath[bands[i]-1]+".gcps");
        p.waitForFinished(999999);

        if(mStopped) break;
        progress += 20/div;
        emit progressed(progress);

        emit logged("Warping band " + QString::number(bands[i]) + ".");
        if(mTransformation == QgsBandAligner::ThinPlateSpline)
        {
            p.start("gdalwarp -multi -t_srs EPSG:4326 -tps " + mInputPath[bands[i]-1] + ".gcps " + mInputPath[bands[i]-1] + ".warped");
            p.waitForFinished(999999);
        }
        else if(mTransformation == QgsBandAligner::Polynomial2)
        {
            p.start("gdalwarp -multi -t_srs EPSG:4326 -order 2 -refine_gcps " + QString::number(mRefinementTolerance) + " " + QString::number(mMinimumGcps) + " " + mInputPath[bands[i]-1] + ".gcps " + mInputPath[bands[i]-1] + ".warped");
            p.waitForFinished(999999);
        }
        else if(mTransformation == QgsBandAligner::Polynomial3)
        {
            p.start("gdalwarp -multi -t_srs EPSG:4326 -order 3 -refine_gcps " + QString::number(mRefinementTolerance) + " " + QString::number(mMinimumGcps) + " " + mInputPath[bands[i]-1] + ".gcps " + mInputPath[bands[i]-1] + ".warped");
            p.waitForFinished(999999);
        }
        else if(mTransformation == QgsBandAligner::Polynomial4)
        {
            p.start("gdalwarp -multi -t_srs EPSG:4326 -order 4 -refine_gcps " + QString::number(mRefinementTolerance) + " " + QString::number(mMinimumGcps) + " " + mInputPath[bands[i]-1] + ".gcps " + mInputPath[bands[i]-1] + ".warped");
            p.waitForFinished(999999);
        }
        else if(mTransformation == QgsBandAligner::Polynomial5)
        {
            p.start("gdalwarp -multi -t_srs EPSG:4326 -order 5 -refine_gcps " + QString::number(mRefinementTolerance) + " " + QString::number(mMinimumGcps) + " " + mInputPath[bands[i]-1] + ".gcps " + mInputPath[bands[i]-1] + ".warped");
            p.waitForFinished(999999);
        }

        QFile f(mInputPath[bands[i]-1] + ".gcps");
        f.remove();

        //-------------------------------------------------------------------------

        progress += 29/div;
        emit progressed(progress);

        delete aligner;
        aligner = NULL;

        emit logged("Alignment for band " + QString::number(bands[i]) + " completed.");
    } 
    if (aligner != NULL) 
        delete aligner;  
    if (mStopped) 
        goto exit;

    //-------------------------------------------------------------------------

    emit logged("Finalizing image structure and metadata...");
    {    
        QString images = "";
        for(int i = 0; i < mInputPath.size(); i++)
        {
            images += mInputPath[i]+".warped ";
        }

        QProcess p;
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

    emit logged("Band alignment for the image completed.");
exit:
    if (mStopped) {
        emit logged("Process determinated.");
    } else {
        emit logged("<<Finished>>");
    }
    progress = 100.0;
    emit progressed(progress);

    if(mInputDataset != NULL)
    {
        GDALClose(mInputDataset);
        mInputDataset = NULL;
    }
}

//-----------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------

/* end of file */
