/***************************************************************************
  testqgsvectoranalyzer.cpp
  --------------------------------------
Date                 : Sun Sep 16 12:22:49 AKDT 2007
Copyright            : (C) 2007 by Gary E. Sherman
Email                : sherman at mrcc dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#undef QT_NO_DEBUG
#include <QtTest>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QObject>
#include <iostream>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QSettings>
#include <QTime>
#include <QDesktopServices>
#include <QDebug>

#include <cpl_string.h>
#include <qgsbandaligner.h>
#include <qgsregioncorrelator.h>

//qgis includes...
#include <qgsapplication.h>
#include <qgsrasterlayer.h>
#include <qgsproviderregistry.h>
#include <qgsmaplayerregistry.h>
//qgis unit test includes
#include <qgsrenderchecker.h>

#include <gsl/gsl_fft_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_blas.h>
#include <math.h>

class TestBandAlignerTool: public QObject
{
    Q_OBJECT;
 
  public slots:
    void logProgress(QString message);
    
  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void init() {};// will be called before each testfunction is executed.
    void cleanup() {};// will be called after every testfunction.

    /** Our tests proper begin here */
    void correlatorTests();
    void verifyDisparityValues();
    void verifyDisparityHalfPixelValues();
    void basicBandAlignerTest();

  private:
    bool render( QString theFileName );
    QString mTestDataDir;
    QgsMapRenderer *mpMapRenderer;
    //QgsRasterLayer *mpRasterLayer;
    QString mReport;
};

/* ************************************************************************* */

void  TestBandAlignerTool::initTestCase()
{
    if (GDALGetDriverCount() == 0)
        GDALAllRegister();  

    // init QGIS's paths - true means that all path will be inited from prefix
    QString qgisPath = QCoreApplication::applicationDirPath();
    QgsApplication::setPrefixPath( INSTALL_PREFIX, true );
    QgsApplication::showSettings();
    // Instantiate the plugin directory so that providers are loaded
    QgsProviderRegistry::instance( QgsApplication::pluginPath() );
    mpMapRenderer = new QgsMapRenderer();
    //create some objects that will be used in all tests...
    mTestDataDir = QString( TEST_DATA_DIR ) + QDir::separator(); //defined in CmakeLists.txt
    //QString mySumbFileName = mTestDataDir + "I0D79/16bit/I0D79_P03_S02_C00_F03_MSSK14K_0.tif";
    //QFileInfo myRasterFileInfo( mySumbFileName );
    //mpRasterLayer = new QgsRasterLayer( myRasterFileInfo.filePath(),
    //                                    myRasterFileInfo.completeBaseName() );
    //mpRasterLayer->setContrastEnhancementAlgorithm( QgsContrastEnhancement::StretchToMinimumMaximum, false );
    //QgsMapLayerRegistry::instance()->addMapLayer( mpRasterLayer );
    mReport += "<h1>Band Aligner Tests</h1>\n";
}

/* ************************************************************************* */

void TestBandAlignerTool::cleanupTestCase()
{
    QString myReportFile = QDir::tempPath() + QDir::separator() + "rastertest.html";
    QFile myFile( myReportFile );
    if ( myFile.open( QIODevice::WriteOnly ) )
    {
      QTextStream myQTextStream( &myFile );
      myQTextStream << mReport;
      myFile.close();  
      QDesktopServices::openUrl( QUrl::fromUserInput(myReportFile) );
    }
    //printf("\nPress ENTER to continue.."); getchar();
}

/* ************************************************************************* */

void TestBandAlignerTool::logProgress(QString message)
{
    printf(" %s\n", message.toAscii().data());
}

/* ************************************************************************* */

void TestBandAlignerTool::basicBandAlignerTest()
{    
    const bool cleanupOutput = true;

    QStringList inputPaths;
    QString outputPath;

    QString dataDir = QString(TEST_DATA_DIR);
    QString id = "I0BDA";
    inputPaths.append(dataDir + "/" + id + "/16bit/" + id + "_P03_S02_C01_F03_MSSK14K_0.tif");
    inputPaths.append(dataDir + "/" + id + "/16bit/" + id + "_P03_S02_C02_F03_MSSK14K_0.tif");
    inputPaths.append(dataDir + "/" + id + "/16bit/" + id + "_P03_S02_C00_F03_MSSK14K_0.tif");

    QString odir = 1 ? QDir::tempPath() : "J:/tmp";    
    outputPath = odir + QDir::separator() + "test_bandaligner-result" + ".tif";
    
    QgsBandAligner *aligner = new QgsBandAligner(inputPaths, outputPath);
    aligner->SetReferenceBand(1);
    aligner->SetBlockSize(127);
#if 0
    QFileInfo o(outputPath);
    QString dispXPath = o.path() + QDir::separator() + o.completeBaseName() + ".xmap" + ".tif";
    QString dispYPath = o.path() + QDir::separator() + o.completeBaseName() + ".ymap" + ".tif";
    aligner->SetDisparityXPath(dispXPath);
    aligner->SetDisparityYPath(dispYPath);
#endif

    QgsProgressMonitor monitor;
    QObject::connect(&monitor, SIGNAL(logged(QString)), this, SLOT(logProgress(QString)));
  
    QTime runTime;   
    runTime.start();

    QgsBandAligner::execute(&monitor, aligner);

    int tm = runTime.elapsed();
    printf(" Execution time was %f seconds", tm/double(1000.0));
    
    // ----- Compare the results and generate the report -----
    QFileInfo myRasterFileInfo( outputPath );
    QgsRasterLayer * mypOutputRasterLayer = new QgsRasterLayer( myRasterFileInfo.filePath(), myRasterFileInfo.completeBaseName() );
    mypOutputRasterLayer->setContrastEnhancementAlgorithm( QgsContrastEnhancement::StretchToMinimumMaximum, false );
    // Register the layer with the registry
    QgsMapLayerRegistry::instance()->addMapLayer( mypOutputRasterLayer );
    QStringList myLayers;
    myLayers << mypOutputRasterLayer->id();
    //myLayers << mpRasterLayer->id();
    qDebug() << myLayers.join(",");
    mpMapRenderer->setLayerSet( myLayers );
    mpMapRenderer->setExtent( mypOutputRasterLayer->extent() );

    QVERIFY( render( "band_aligner" ) );

    /* Remove the files we created on successfull completion */
    if (cleanupOutput) {
        unlink(outputPath.toAscii().data());
#if 0
        unlink(dispXPath.toAscii().data());
        unlink(dispYPath.toAscii().data());
#endif
    }
}

/* ************************************************************************* */

void copyBand(GDALRasterBand *srcRasterBand, GDALRasterBand *dstRasterBand, int xOffset = 0, int yOffset = 0)
{
    Q_CHECK_PTR(srcRasterBand);
    Q_CHECK_PTR(dstRasterBand);

    int width = srcRasterBand->GetXSize();
    int height = srcRasterBand->GetYSize();
    GDALDataType dataType = srcRasterBand->GetRasterDataType();
    int dataSize = GDALGetDataTypeSize(dataType) / 8;

    Q_ASSERT(dataSize > 0);
    
    void *scanline = calloc(width + abs(xOffset), dataSize);
    
    // Calculate the start pointer for the read & write buffers
    void *scanline_write = (void *)((intptr_t)scanline + dataSize * (xOffset > 0 ? xOffset : 0));
    void *scanline_read = (void *)((intptr_t)scanline + dataSize * (xOffset >= 0 ? 0 : -xOffset));
    
    for (int y = 0; y < height; ++y)
    {
        if (0 <= (y + yOffset) && (y + yOffset) < height) {
            srcRasterBand->RasterIO(GF_Read, 0, y + yOffset, width, 1, scanline_read, width, 1, dataType, 0, 0);
        } else {
            memset(scanline_read, 0, width * dataSize);
        }        
        dstRasterBand->RasterIO(GF_Write, 0, y, width, 1, scanline_write, width, 1, dataType, 0, 0);
    }

    free(scanline);
}

/* ************************************************************************* */

#ifdef _MSC_VER
#define API __declspec(dllimport)
#else
#define API
#endif

extern "C" API int QgsRegionCorrelator_test1();

void TestBandAlignerTool::correlatorTests()
{
    if (0) return;

    Q_ASSERT(QgsRegionCorrelator_test1() == 0);
}

/* ************************************************************************* */

void TestBandAlignerTool::verifyDisparityValues()
{
    bool cleanupOutput = true;

    QStringList inputPaths;
    QString outputPath;
    QString tmpBand;
    
    QString dataDir = QString(TEST_DATA_DIR);
    QString id = "I0BDA";

    inputPaths.append(dataDir + "/" + id + "/16bit/" + id + "_P03_S02_C01_F03_MSSK14K_0.tif");
    //inputPaths.append(dataDir + "/" + id + "/16bit/" + id + "_P03_S02_C02_F03_MSSK14K_0.tif");
    //inputPaths.append(dataDir + "/" + id + "/16bit/" + id + "_P03_S02_C00_F03_MSSK14K_0.tif");

    outputPath = 1 ? QDir::tempPath() : QString("J:/tmp");
    outputPath = outputPath + QDir::separator() + "test_bandaligner_verifier" + ".tif";
    
    int offsetX = +15;
    int offsetY = +30;

    tmpBand = QDir::tempPath() + QDir::separator() + "test_bandaligner_offsetband" + ".tif";
    { 
        /* create a copy that is shifted to test band alignment */

        GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");
        GDALDataset *dsi = (GDALDataset*) GDALOpen(inputPaths[0].toAscii().data(), GA_ReadOnly);
        GDALDataset *dso = driver->CreateCopy(tmpBand.toAscii().data(), dsi, FALSE, NULL, NULL, NULL);
    
        copyBand(dsi->GetRasterBand(1), dso->GetRasterBand(1), offsetX, offsetY);

        GDALClose(dso);
        GDALClose(dsi);        
    }

    QStringList list;
    list.append(inputPaths[0]);
    list.append(tmpBand);

    QgsBandAligner *aligner = new QgsBandAligner(list, outputPath);
    aligner->SetReferenceBand(1);
    aligner->SetBlockSize(127);

    QFileInfo o(outputPath);

    QString dispXPath = o.path() + QDir::separator() + o.completeBaseName() + ".xmap" + ".tif";
    aligner->SetDisparityXPath(dispXPath);

    QString dispYPath = o.path() + QDir::separator() + o.completeBaseName() + ".ymap" + ".tif";
    aligner->SetDisparityYPath(dispYPath);

    QString dispGridPath = o.path() + QDir::separator() + o.completeBaseName() + ".grid" + ".tif";        
    aligner->SetDisparityGridPath(dispGridPath);

    QgsProgressMonitor monitor;
    QObject::connect(&monitor, SIGNAL(logged(QString)), this, SLOT(logProgress(QString)));    

    QTime runTime;   
    runTime.start();

    QgsBandAligner::execute(&monitor, aligner);

    int tm = runTime.elapsed();
    printf(" Execution time was %f seconds", tm/double(1000.0));

    {
        // Calculate statistics on disparity map
        GDALDataset *ds;
        double min, max, mean, stddev, errX, errY;

        printf("\nOFFSET:\n");
        printf("  X      = %4d\n", offsetX);
        printf("  Y      = %4d\n", offsetY);
            
        ds = (GDALDataset*) GDALOpen(dispXPath.toAscii().data(), GA_ReadOnly);
        ds->GetRasterBand(1)->ComputeStatistics(false, &min, &max, &mean, &stddev, NULL, NULL);
        GDALClose(ds);

        errX = fabs(offsetX - mean);

        printf("\nDISPARITY MAP X:\n");
        printf("  min    = %7.3f\n", min);
        printf("  max    = %7.3f\n", max);
        printf("  mean   = %10f\n", mean);
        printf("  stddev = %10f\n", stddev);
        printf("  err    = %10f\n", errX);

        ds = (GDALDataset*) GDALOpen(dispYPath.toAscii().data(), GA_ReadOnly);
        ds->GetRasterBand(1)->ComputeStatistics(false, &min, &max, &mean, &stddev, NULL, NULL);
        GDALClose(ds);

        errY = fabs(offsetY - mean);

        printf("\nDISPARITY MAP Y:\n");
        printf("  min    = %7.3f\n", min);
        printf("  max    = %7.3f\n", max);
        printf("  mean   = %10f\n", mean);
        printf("  stddev = %10f\n", stddev);
        printf("  err    = %10f\n", errY);

        //printf("\nPress ENTER to continue.."); getchar();

        Q_ASSERT(errX < 0.05);
        Q_ASSERT(errY < 0.05);
    }    
    
    // Remove temporary file afterwards 
    if (cleanupOutput) {
        unlink(tmpBand.toAscii().data());
        unlink(dispGridPath.toAscii().data());
        unlink(dispXPath.toAscii().data());
        unlink(dispYPath.toAscii().data());
        unlink(outputPath.toAscii().data());
    }
}

/* ************************************************************************* */

void TestBandAlignerTool::verifyDisparityHalfPixelValues()
{
    bool cleanupOutput = true;

    QStringList inputPaths;
    QString outputPath;

    QString dataDir = QString(TEST_DATA_DIR);
    QString id = "I0BDA";
    inputPaths.append(dataDir + "/" + id + "/other/" + "Beeld 0.tif");
    inputPaths.append(dataDir + "/" + id + "/other/" + "Beeld 1 (1.5 px na regs; 2.5 px afwaarts).tif");
    
    for (int i = 0; i < inputPaths.size(); ++i) {
        // Ensure that all the input files exist
        if (!QFile(inputPaths[i]).exists())
            return; 
    }

    outputPath = 1 ? QDir::tempPath() : QString("J:/tmp");
    outputPath = outputPath + QDir::separator() + "test_bandaligner_half_pixel_verifier" + ".tif";
    
    QgsBandAligner *aligner = new QgsBandAligner(inputPaths, outputPath);
    aligner->SetReferenceBand(1);
    aligner->SetBlockSize(101);

    QFileInfo o(outputPath);

    QString dispXPath = o.path() + QDir::separator() + o.completeBaseName() + ".xmap" + ".tif";
    aligner->SetDisparityXPath(dispXPath);

    QString dispYPath = o.path() + QDir::separator() + o.completeBaseName() + ".ymap" + ".tif";
    aligner->SetDisparityYPath(dispYPath);

    QString dispGridPath = o.path() + QDir::separator() + o.completeBaseName() + ".grid" + ".tif";        
    aligner->SetDisparityGridPath(dispGridPath);

    QgsProgressMonitor monitor;
    QObject::connect(&monitor, SIGNAL(logged(QString)), this, SLOT(logProgress(QString)));    

    QTime runTime;   
    runTime.start();

    QgsBandAligner::execute(&monitor, aligner);

    int tm = runTime.elapsed();
    printf(" Execution time was %f seconds", tm/double(1000.0));

    double offsetX = -1.5; // pixels to the right
    double offsetY = -2.5; // pixels downwards
    {
        // Calculate statistics on disparity map
        GDALDataset *ds;
        double min, max, mean, stddev, errX, errY;

        printf("\nOFFSET:\n");
        printf("  X      = %4.2f\n", offsetX);
        printf("  Y      = %4.2f\n", offsetY);
            
        ds = (GDALDataset*) GDALOpen(dispXPath.toAscii().data(), GA_ReadOnly);
        ds->GetRasterBand(1)->ComputeStatistics(false, &min, &max, &mean, &stddev, NULL, NULL);
        GDALClose(ds);

        errX = fabs(offsetX - mean);

        printf("\nDISPARITY MAP X:\n");
        printf("  min    = %7.3f\n", min);
        printf("  max    = %7.3f\n", max);
        printf("  mean   = %10f\n", mean);
        printf("  stddev = %10f\n", stddev);
        printf("  err    = %10f\n", errX);

        ds = (GDALDataset*) GDALOpen(dispYPath.toAscii().data(), GA_ReadOnly);
        ds->GetRasterBand(1)->ComputeStatistics(false, &min, &max, &mean, &stddev, NULL, NULL);
        GDALClose(ds);

        errY = fabs(offsetY - mean);

        printf("\nDISPARITY MAP Y:\n");
        printf("  min    = %7.3f\n", min);
        printf("  max    = %7.3f\n", max);
        printf("  mean   = %10f\n", mean);
        printf("  stddev = %10f\n", stddev);
        printf("  err    = %10f\n", errY);

        //printf("\nPress ENTER to continue.."); getchar();

        Q_ASSERT(errX < 0.05);
        Q_ASSERT(errY < 0.05);
    }    
    
    // Remove temporary file afterwards 
    if (cleanupOutput) {
        unlink(dispGridPath.toAscii().data());
        unlink(dispXPath.toAscii().data());
        unlink(dispYPath.toAscii().data());
        unlink(outputPath.toAscii().data());
    }
}

/* ************************************************************************* */

//
// Helper methods
//

bool TestBandAlignerTool::render( QString theTestType )
{
  mReport += "<h2>" + theTestType + "</h2>\n";
  QString myDataDir( TEST_DATA_DIR ); //defined in CmakeLists.txt
  QString myTestDataDir = myDataDir + QDir::separator();
  QgsRenderChecker myChecker;
  myChecker.setExpectedImage( myTestDataDir + "expected_" + theTestType + ".png" );
  myChecker.setMapRenderer( mpMapRenderer );
  bool myResultFlag = myChecker.runTest( theTestType );
  mReport += "\n\n\n" + myChecker.report();
  return myResultFlag;
}

QTEST_MAIN( TestBandAlignerTool )
#include "tools/moc_qgsbandaligner_test.cxx"

