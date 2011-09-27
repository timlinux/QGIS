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
 
  public slots:
    void logProgress(QString message);
    
  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void init() {};// will be called before each testfunction is executed.
    void cleanup() {};// will be called after every testfunction.

    /** Our tests proper begin here */
    void basicBandAlignerTest();

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

void TestBandAlignerTool::logProgress(QString message)
{
    printf(" %s\n", message.toAscii().data());
}

void TestBandAlignerTool::basicBandAlignerTest()
{
    const bool cleanupOutput = true;

    QStringList inputPaths;
    QString outputPath;

    QString id = "I0BDA";
    inputPaths.append(QString(TEST_DATA_DIR) + "/" + id + "/16bit/" + id + "_P03_S02_C01_F03_MSSK14K_0.tif");
    inputPaths.append(QString(TEST_DATA_DIR) + "/" + id + "/16bit/" + id + "_P03_S02_C02_F03_MSSK14K_0.tif");
    inputPaths.append(QString(TEST_DATA_DIR) + "/" + id + "/16bit/" + id + "_P03_S02_C00_F03_MSSK14K_0.tif");
    //outputPath = QDir::tempPath() + "test_bandaligner-result" + ".tif";
    outputPath = "J:/tmp/test3.tif";

    QgsBandAligner *aligner = new QgsBandAligner(inputPaths, outputPath);
    aligner->SetReferenceBand(1);
    aligner->SetBlockSize(125);
#if 1
    QFileInfo o(outputPath);
    QString dispXPath = o.path() + QDir::separator() + o.completeBaseName() + ".xmap" + ".tif";
    QString dispYPath = o.path() + QDir::separator() + o.completeBaseName() + ".ymap" + ".tif";
    aligner->SetDisparityXPath(dispXPath);
    aligner->SetDisparityYPath(dispYPath);
#endif

    QgsProgressMonitor monitor;
    QObject::connect(&monitor, SIGNAL(logged(QString)), this, SLOT(logProgress(QString)));
  
    QgsBandAligner::execute(&monitor, aligner);
    
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
#if 1
        unlink(dispXPath.toAscii().data());
        unlink(dispYPath.toAscii().data());
#endif
    }
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

