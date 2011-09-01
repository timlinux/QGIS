/***************************************************************************
     testqgsvectorfilewriter.cpp
     --------------------------------------
    Date                 : Frida  Nov 23  2007
    Copyright            : (C) 2007 by Tim Sutton
    Email                : tim@linfiniti.com
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


//qgis includes...
#include <qgsapplication.h>
#include <qgscloudanalyzer.h>
//qgis unit test includes
#include <qgsrenderchecker.h>


/** \ingroup UnitTests
 * This is a unit test for the QgsCloudAnalyzer class.
 */
class QgsCloudAnalyzerTest: public QObject
{
    Q_OBJECT;
  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void init() {};// will be called before each testfunction is executed.
    void cleanup() {};// will be called after every testfunction.

    void analyzeTest();
  private:
    bool render( QString theFileName );
    QString mTestDataDir;
    QgsMapRenderer * mpMapRenderer;
    QString mReport;
};

//runs before all tests
void QgsCloudAnalyzerTest::initTestCase()
{
  // init QGIS's paths - true means that all path will be inited from prefix
  QString qgisPath = QCoreApplication::applicationDirPath();
  QgsApplication::setPrefixPath( INSTALL_PREFIX, true );
  QgsApplication::showSettings();
  //create some objects that will be used in all tests...
  mTestDataDir = QString( TEST_DATA_DIR ) + QDir::separator(); //defined in CmakeLists.txt
  mReport += "<h1>Cloud Analyzer Tests</h1>\n";
}
//runs after all tests
void QgsCloudAnalyzerTest::cleanupTestCase()
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


void QgsCloudAnalyzerTest::analyzeTest()
{
  QString myTempPath = QDir::tempPath() + QDir::separator() ;
  QString myDataDir( TEST_DATA_DIR ); //defined in CmakeLists.txt
  QString myFileName = myDataDir + "/I0D79/16bit/I0D79_P03_S02_C00_F03_MSSK14K_0.tif";
  QgsCloudAnalyzer myAnalyzer( myFileName );
  myAnalyzer.analyze();
  //QFile::remove( myTempPath + "landsat.tif.ovr" );
  //QFile::copy( mTestDataDir + "landsat.tif", myTempPath + "landsat.tif" );
  //QFileInfo myRasterFileInfo( myTempPath + "landsat.tif" );
  //QgsRasterLayer * mypLayer = new QgsRasterLayer( myRasterFileInfo.filePath(),
  //    myRasterFileInfo.completeBaseName() );

  ///QgsMapLayerRegistry::instance()->addMapLayer( mypLayer, false );
  //QgsMapLayerRegistry::instance()->removeMapLayer( mypLayer->id() );
  //cleanup
  //delete mypLayer;
  //QVERIFY( render( "raster" ) );
}

//
// Helper methods
//


bool QgsCloudAnalyzerTest::render( QString theTestType )
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


QTEST_MAIN( QgsCloudAnalyzerTest )
#include "moc_qgscloudanalyzer_test.cxx"

