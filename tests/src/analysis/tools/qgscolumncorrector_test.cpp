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

//header for class being tested
#include <qgsgeometryanalyzer.h>
#include <qgsapplication.h>
#include <qgsproviderregistry.h>

#include <qgsimageprocessor.h>

//qgis includes...
#include <qgsapplication.h>
#include <qgsrasterlayer.h>
#include <qgsproviderregistry.h>
#include <qgsmaplayerregistry.h>
//qgis unit test includes
#include <qgsrenderchecker.h>

class TestColumnCorrectorTool: public QObject
{
    Q_OBJECT;
  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void init() {};// will be called before each testfunction is executed.
    void cleanup() {};// will be called after every testfunction.

    /** Our tests proper begin here */
    void simpleFirstTest();

  private:
    bool render( QString theFileName );
    QString mTestDataDir;
    QgsMapRenderer *mpMapRenderer;
    //QgsRasterLayer *mpRasterLayer;
    QString mReport;
};

void  TestColumnCorrectorTool::initTestCase()
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
    mReport += "<h1>Column Corrector Tests</h1>\n";
}

void TestColumnCorrectorTool::cleanupTestCase()
{
    QString myReportFile = QDir::tempPath() + QDir::separator() + "rastertest.html";
    QFile myFile( myReportFile );
    if ( myFile.open( QIODevice::WriteOnly ) )
    {
      QTextStream myQTextStream( &myFile );
      myQTextStream << mReport;
      myFile.close();
      QDesktopServices::openUrl( "file://" + myReportFile );
    }
}

void TestColumnCorrectorTool::simpleFirstTest()
{
    //QStringList inputPaths;
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C00_F03_MSSK14K_0.tif");
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C01_F03_MSSK14K_0.tif");
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C02_F03_MSSK14K_0.tif");

	QString outputPath = "c:/tmp/test_columncorrector-result.tif";

    QString inputPath = QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C00_F03_MSSK14K_0.tif";

    QgsImageProcessor *m = new QgsImageProcessor();

    m->setInputPath(inputPath);
    m->setOutputPath(outputPath);
    m->setCheckPath("");
    m->setInFilePath("");
    m->setOutFilePath("");
    m->setThresholds(1.0, 0.0);

    m->start();
    while (m->isRunning())
    {
        QTest::qSleep(1000);
    }
    m->stop();

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

    QVERIFY( render( "column_corrector" ) );
}

//
// Helper methods
//

bool TestColumnCorrectorTool::render( QString theTestType )
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

QTEST_MAIN( TestColumnCorrectorTool )
#include "tools/moc_qgscolumncorrector_test.cxx"

