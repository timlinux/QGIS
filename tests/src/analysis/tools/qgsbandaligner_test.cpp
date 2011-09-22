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

//qgis includes...
#include <qgsapplication.h>
#include <qgsrasterlayer.h>
#include <qgsproviderregistry.h>
#include <qgsmaplayerregistry.h>
//qgis unit test includes
#include <qgsrenderchecker.h>

class TestBandAlignerTool: public QObject
{
    Q_OBJECT;
  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void init() {};// will be called before each testfunction is executed.
    void cleanup() {};// will be called after every testfunction.

    /** Our tests proper begin here */
    void simpleFirstTest();

  private: // disabled tests
    void createDisparityMap();
    //void combineBandsInTiffTest();
    void concurTest();
    //void combineBandsInTiffWithOffsetTest();

  private:
    bool render( QString theFileName );
    QString mTestDataDir;
    QgsMapRenderer *mpMapRenderer;
    //QgsRasterLayer *mpRasterLayer;
    QString mReport;
};

void  TestBandAlignerTool::initTestCase()
{
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
}

#if 0
void TestBandAlignerTool::imageSplit()
{
    GDALRasterBand *srcRasterBand = srcDataSet->GetRasterBand(srcBand);
    GDALRasterBand *dstRasterBand = dstDataSet->GetRasterBand(dstBand);

    int width = srcDataSet->GetRasterXSize();
    int height = srcDataSet->GetRasterYSize();
    GDALDataType dataType = srcRasterBand->GetRasterDataType();
    
    int *scanline = (int *)malloc(width * sizeof(int));

    for (int y = 0; y < height; ++y) 
    {
        srcRasterBand->RasterIO(GF_Read, 0, y, width, 1, scanline, width, 1, dataType, 0, 0);
        dstRasterBand->RasterIO(GF_Write, 0, y+rowOffset, width, 1, scanline, width, 1, dataType, 0, 0);
    }

    free(scanline);
}
#endif

#if 0
void copyBand(GDALRasterBand *srcRasterBand, GDALRasterBand *dstRasterBand, int xOffset = 0, int yOffset = 0)
{
    int width = srcRasterBand->GetXSize();
    int height = srcRasterBand->GetYSize();
    GDALDataType dataType = srcRasterBand->GetRasterDataType();
    
    int *scanline = (int *)calloc(width + abs(xOffset), sizeof(int));
    
    int *scanline_read = &scanline[(xOffset >= 0) ? xOffset : 0];
    int *scanline_write = &scanline[(xOffset >= 0) ? 0 : -xOffset];
    
    for (int y = 0; y < height; ++y)
    {
        if (0 <= (y + yOffset) && (y + yOffset) < height) {
            srcRasterBand->RasterIO(GF_Read, 0, y + yOffset, width, 1, scanline_read, width, 1, dataType, 0, 0);
        } else {
            memset(scanline_read, 0, width * sizeof(scanline_read[0]));
        }        
        dstRasterBand->RasterIO(GF_Write, 0, y, width, 1, scanline_write, width, 1, dataType, 0, 0);
    }

    free(scanline);
}
#endif

#if 0
void TestBandAlignerTool::combineBandsInTiffWithOffsetTest()
{
    //__asm int 3; // Hard code a debugger breakpoint in C++

    QStringList inputPaths;
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C00_F03_MSSK14K_0.tif");
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C01_F03_MSSK14K_0.tif");
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C02_F03_MSSK14K_0.tif");

    QString outputPath = "J:/tmp/test4b.tif";

    if (GDALGetDriverCount() == 0)
        GDALAllRegister();  

    QList<GDALDataset*> inputDataSets;        
    inputDataSets.append((GDALDataset*)GDALOpen(inputPaths[0].toAscii().data(), GA_ReadOnly));
    inputDataSets.append((GDALDataset*)GDALOpen(inputPaths[1].toAscii().data(), GA_ReadOnly));
    inputDataSets.append((GDALDataset*)GDALOpen(inputPaths[2].toAscii().data(), GA_ReadOnly));

    int mWidth = inputDataSets[0]->GetRasterXSize();
    int mHeight = inputDataSets[0]->GetRasterYSize();
    GDALDataType mDataType = inputDataSets[0]->GetRasterBand(1)->GetRasterDataType();

    mHeight += 45 * 2; // thumb suck value
    
    GDALDriver *driver = inputDataSets[0]->GetDriver();    
    GDALDataset *outputDataset = driver->Create(outputPath.toAscii().data(), mWidth, mHeight, inputDataSets.size(), mDataType, 0);    
    char **outputMetadata = driver->GetMetadata();        

    for (int b = 0; b < inputDataSets.size(); b++) 
    {        
        copyBand(inputDataSets[b], 1, outputDataset, 1+b, 44*(inputDataSets.size()-(1+b)));
    }
    
    outputDataset->SetMetadata(outputMetadata);

    GDALClose(outputDataset);
    
    for (int i = 0; i < inputDataSets.size(); ++i) 
    {
        GDALClose(inputDataSets[i]);
    }
}
#endif

void TestBandAlignerTool::createDisparityMap()
{
#if 0
    QStringList inputPaths;
    //inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C00_F03_MSSK14K_0.tif");
    //inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C01_F03_MSSK14K_0.tif");
    //inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C02_F03_MSSK14K_0.tif");
    //QString outputPath = "J:/tmp/test5.tif";
    //QString dispXPath = "J:/tmp/test5-x.tif";
    //QString dispYPath = "J:/tmp/test5-y.tif";
    //QString dispInterPath = "J:/tmp/test5-subset.tif";

    inputPaths.append("J:/tmp/fruit-basket.1024x768-b.tif");
    inputPaths.append("J:/tmp/fruit-basket.1024x768-a.tif");
    inputPaths.append("J:/tmp/fruit-basket.1024x768-c.tif");
    QString outputPath = "J:/tmp/test6.tif";
    QString dispXPath = "J:/tmp/test6-x.tif";
    QString dispYPath = "J:/tmp/test6-y.tif";
    QString dispInterPath = "J:/tmp/test6-s.tif";

    QList<GDALRasterBand *> mBands; // inputs
    int nRefBand;

    if (GDALGetDriverCount() == 0)
        GDALAllRegister();
    
    //{
        GDALDataset *inputDataset1 = (GDALDataset*)GDALOpen(inputPaths[0].toAscii().data(), GA_ReadOnly);
        Q_CHECK_PTR(inputDataset1);

        mBands.append(inputDataset1->GetRasterBand(1)); // first band

        GDALDataset* inputRefDataset = (GDALDataset*)GDALOpen(inputPaths[1].toAscii().data(), GA_ReadOnly);
        Q_CHECK_PTR(inputRefDataset);

        mBands.append(inputRefDataset->GetRasterBand(1)); // second band

        GDALDataset *inputDataset2 = (GDALDataset*)GDALOpen(inputPaths[2].toAscii().data(), GA_ReadOnly);
        Q_CHECK_PTR(inputDataset2);

        mBands.append(inputDataset2->GetRasterBand(1)); // third band
        
        nRefBand = 1; // second band
    //}

    //----------------------------------------------------------------

    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");    

    //----------------------------------------------------------------

    if (mBands.size() < 2) {
        printf("Not enough bands to continue, quiting\n");
        return; // nothing to do
    }

    Q_ASSERT(0 <= nRefBand && nRefBand < mBands.size());

    int mWidth = mBands[nRefBand]->GetXSize();
    int mHeight = mBands[nRefBand]->GetYSize();

    /* Delete to output file. Just in case there is one lying around that is faulty */
    unlink(outputPath.toAscii().data()); // FIXME: append bands

    GDALDataset *outputDataSet = driver->Create(outputPath.toAscii().data(), mWidth, mHeight, 3, /*GDT_UInt16*/GDT_Byte, 0);
    Q_CHECK_PTR(outputDataSet);

    QTime stageTimer;

    // ------------------------------------

    for (int i = 1; i < mBands.size(); ++i)
    {
        if (i == nRefBand) {
            /* Copy the reference band over to the output file */
            copyBand(mBands[i], outputDataSet->GetRasterBand(1+i));
            continue;
        }

        Q_CHECK_PTR(mBands[nRefBand]);
        Q_CHECK_PTR(mBands[i]);

        GDALDataset *disparityDataset;
        GDALDataset *disparityXDataSet;
        GDALDataset *disparityYDataSet;

        StatisticsState stats[2];

        // ------------------------------------
        {    
            stageTimer.start();

        disparityDataset = scanDisparityMap(mBands[nRefBand], mBands[i], dispInterPath, stats, 61);

            printf("Runtime for scanDisparityMap was %d ms\n", stageTimer.elapsed());
        }
        // ------------------------------------
        {
            stageTimer.start();

        eliminateDisparityMap(disparityDataset->GetRasterBand(1), stats);

            printf("Runtime for eliminateDisparityMap was %d ms\n", stageTimer.elapsed());
        }
        // ------------------------------------
        {
            unlink(dispXPath.toAscii().data()); 
            unlink(dispYPath.toAscii().data());
    
            disparityXDataSet = driver->Create(dispXPath.toAscii().data(), mWidth, mHeight, 1, GDT_Float32, 0);
            Q_CHECK_PTR(disparityXDataSet);

            disparityYDataSet = driver->Create(dispYPath.toAscii().data(), mWidth, mHeight, 1, GDT_Float32, 0);
            Q_CHECK_PTR(disparityYDataSet);

            stageTimer.start();

        estimateDisparityMap(
            disparityDataset->GetRasterBand(1),
            disparityXDataSet->GetRasterBand(1), 
            disparityYDataSet->GetRasterBand(1));

            printf("Runtime for estimateDisparityMap was %d ms\n", stageTimer.elapsed());
        }
        // ------------------------------------

        GDALClose(disparityDataset);

        // ------------------------------------
        {
            stageTimer.start();

        applyDisparityMap(
            mBands[i],
            disparityXDataSet->GetRasterBand(1), 
            disparityYDataSet->GetRasterBand(1), 
            outputDataSet->GetRasterBand(1+i) );
            
            int applyRunTimeMs = stageTimer.elapsed();
            printf("Runtime for applyDisparityMap was %d ms\n", applyRunTimeMs);
        }
        // ------------------------------------

        GDALClose(disparityYDataSet);
        GDALClose(disparityXDataSet);
    }
#if 0   
    // ------------------------------------
    {
        unlink(dispXPath.toAscii().data()); 
        unlink(dispYPath.toAscii().data());

        GDALDataset *disparityXDataSet = driver->Create(dispXPath.toAscii().data(), mWidth, mHeight, 1, GDT_Float32, 0);
        Q_CHECK_PTR(disparityXDataSet);

        GDALDataset *disparityYDataSet = driver->Create(dispYPath.toAscii().data(), mWidth, mHeight, 1, GDT_Float32, 0);
        Q_CHECK_PTR(disparityYDataSet);        

        GDALDataset* disparityDataset = (GDALDataset*)GDALOpen(dispInterPath.toAscii().data(), GA_Update);
        Q_CHECK_PTR(disparityDataset);
        Q_ASSERT(disparityDataset->GetRasterBand(1)->GetRasterDataType() == GDT_CFloat32);

        stageTimer.start();

        estimateDisparityMap(       
            disparityDataset->GetRasterBand(1),
            disparityXDataSet->GetRasterBand(1), 
            disparityYDataSet->GetRasterBand(1));

        int estimateRunTimeMs = stageTimer.elapsed();
        printf("Runtime for estimateDisparityMap was %d ms\n", estimateRunTimeMs);

        GDALClose(disparityDataset);

        GDALClose(disparityYDataSet);
        GDALClose(disparityXDataSet);
    }
    // ------------------------------------
    {
        GDALDataset *disparityXDataSet = (GDALDataset*)GDALOpen(dispXPath.toAscii().data(), GA_ReadOnly);
        GDALDataset *disparityYDataSet = (GDALDataset*)GDALOpen(dispYPath.toAscii().data(), GA_ReadOnly);

        stageTimer.start();

        applyDisparityMap(
            inputDataset->GetRasterBand(1),
            disparityXDataSet->GetRasterBand(1), 
            disparityYDataSet->GetRasterBand(1), 
            outputDataSet->GetRasterBand(1));
        
        int applyRunTimeMs = stageTimer.elapsed();
        printf("Runtime for applyDisparityMap was %d ms\n", applyRunTimeMs);

        GDALClose(disparityYDataSet);
        GDALClose(disparityXDataSet);
    }

    GDALClose(inputDataset);
    // ------------------------------------
    { // begin: second band
        /*GDALDataset* */ inputDataset = (GDALDataset*)GDALOpen(inputPaths[2].toAscii().data(), GA_ReadOnly);
        Q_CHECK_PTR(inputDataset);

        scanDisparityMap(inputRefDataset, inputDataset, dispInterPath, stats, 61);

        GDALDataset* disparityDataset = (GDALDataset*)GDALOpen(dispInterPath.toAscii().data(), GA_Update);
        Q_CHECK_PTR(disparityDataset);

        eliminateDisparityMap(disparityDataset->GetRasterBand(1), stats);

        unlink(dispXPath.toAscii().data()); 
        unlink(dispYPath.toAscii().data());

        GDALDataset *disparityXDataSet = driver->Create(dispXPath.toAscii().data(), mWidth, mHeight, 1, GDT_Float32, 0);
        Q_CHECK_PTR(disparityXDataSet);

        GDALDataset *disparityYDataSet = driver->Create(dispYPath.toAscii().data(), mWidth, mHeight, 1, GDT_Float32, 0);
        Q_CHECK_PTR(disparityYDataSet);        

        estimateDisparityMap(       
            disparityDataset->GetRasterBand(1),
            disparityXDataSet->GetRasterBand(1), 
            disparityYDataSet->GetRasterBand(1));

        GDALClose(disparityDataset); // finished with this dataset

        applyDisparityMap(
            inputDataset->GetRasterBand(1),
            disparityXDataSet->GetRasterBand(1), 
            disparityYDataSet->GetRasterBand(1), 
            outputDataSet->GetRasterBand(3));

        GDALClose(disparityYDataSet);
        GDALClose(disparityXDataSet);

        GDALClose(inputDataset);
    } // end: second band
    // ------------------------------------
#endif

    GDALClose(outputDataSet);

    GDALClose(inputDataset2);
    GDALClose(inputRefDataset);
    GDALClose(inputDataset1);
#endif
}

void TestBandAlignerTool::simpleFirstTest()
{
    //QStringList inputPaths;
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C00_F03_MSSK14K_0.tif");
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C01_F03_MSSK14K_0.tif");
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C02_F03_MSSK14K_0.tif");
    //QString outputPath = QDir::tempPath() + "test_bandaligner-result" + ".tif";

    QStringList inputPaths;
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C01_F03_MSSK14K_0.tif");
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C02_F03_MSSK14K_0.tif");
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C00_F03_MSSK14K_0.tif");

    QString outputPath = "J:/tmp/test3.tif";
    QString dispXPath = "J:/tmp/test3-x.tif";
    QString dispYPath = "J:/tmp/test3-y.tif";
    QString dispGridPath = "J:/tmp/test3-grid.tif";

    QgsBandAligner *aligner = new QgsBandAligner(inputPaths, outputPath);
    aligner->SetDisparityXPath(dispXPath);
    aligner->SetDisparityXPath(dispYPath);

    QgsProgressMonitor monitor;

    QgsBandAligner::execute(&monitor, aligner);
    

    // ----- Compare the results and generate the report -----
#if 0
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
#endif
}

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

