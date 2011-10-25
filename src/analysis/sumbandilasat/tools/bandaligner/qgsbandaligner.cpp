#include "qgsprogressmonitor.h"
#include "qgsbandaligner.h"

#include <QDir>
#include <QDebug>
#include <QFuture>
#include <qgslogger.h>
#include <QtCore>
#include <gdal_priv.h>

#include <QThread>
#include <QMutex>
#include <math.h>
#include "qgscomplex.h"
#include "qgsregioncorrelator.h"
#include <iostream>
#include "gsl/gsl_statistics_double.h"
#include "gsl/gsl_blas.h"

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
        self->executeDisparity(*monitor);
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

    QTime stopwatch;

    if (monitor.IsCanceled()) 
        goto exit1;
    
    monitor.Log("Starting band alignment on the image ...");    
    stopwatch.start();

    QgsImageAligner::performImageAlignment(monitor, 
        mOutputPath, mDisparityXPath, mDisparityYPath, 
        mDisparityGridPath, inputBands, mReferenceBand, mBlockSize);
      
    monitor.Log(QString("Band alignment completed in %1 seconds.").arg(stopwatch.elapsed()/1000));
    
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

//-----------------------------------------------------------------------------------

/* end of file */
