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

//header for class being tested
#include <qgsgeometryanalyzer.h>
#include <qgsapplication.h>
#include <qgsproviderregistry.h>

#include <qgsimageprocessor.h>

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

    //void singleToMulti(  );
    //void multiToSingle(  );
    //void extractNodes(  );
    //void polygonsToLines(  );
    //void exportGeometryInfo(  );
    //void simplifyGeometry(  );
    //void polygonCentroids(  );
    //void layerExtent(  );

  public:
    void logReceived(QString message);
    void progressReceived(double progress);

  private:
    //QgsGeometryAnalyzer mAnalyzer;
    //QgsVectorLayer * mpLineLayer;
    //QgsVectorLayer * mpPolyLayer;
    //QgsVectorLayer * mpPointLayer;
    
};

void  TestColumnCorrectorTool::initTestCase()
{
  //
  // Runs once before any tests are run
  //
  // init QGIS's paths - true means that all path will be inited from prefix
  QString qgisPath = QCoreApplication::applicationDirPath ();
  QgsApplication::setPrefixPath(INSTALL_PREFIX, true);
  QgsApplication::showSettings();
  // Instantiate the plugin directory so that providers are loaded    
  QgsProviderRegistry::instance(QgsApplication::pluginPath());
  //create some objects that will be used in all tests...
  //create a map layer that will be used in all tests...
}

void TestColumnCorrectorTool::cleanupTestCase()
{

}

void TestColumnCorrectorTool::logReceived(QString message)
{
  //emit logged(message);
}

void TestColumnCorrectorTool::progressReceived(double progress)
{
  //emit progressed(progress);
}


void TestColumnCorrectorTool::simpleFirstTest()
{
    //QStringList inputPaths;
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C00_F03_MSSK14K_0.tif");
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C01_F03_MSSK14K_0.tif");
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C02_F03_MSSK14K_0.tif");

    QString outputPath = "/home/fgretief/w/tmp/test_columncorrector-result.tif";

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
        sleep(1);
    }
    m->stop();
}


/*
void TestQgsVectorAnalyzer::simplifyGeometry(  )
{
  QString myTmpDir = QDir::tempPath() + QDir::separator() ;
  QString myFileName = myTmpDir +  "simplify_layer.shp";
  QVERIFY( mAnalyzer.simplify( mpLineLayer,
                             myFileName,
                             1.0 ) );
}

void TestQgsVectorAnalyzer::polygonCentroids(  )
{
  QString myTmpDir = QDir::tempPath() + QDir::separator() ;
  QString myFileName = myTmpDir +  "centroid_layer.shp";
  QVERIFY( mAnalyzer.centroids( mpPolyLayer, myFileName ) );
}

void TestQgsVectorAnalyzer::layerExtent(  )
{
  QString myTmpDir = QDir::tempPath() + QDir::separator() ;
  QString myFileName = myTmpDir +  "extent_layer.shp";
  QVERIFY( mAnalyzer.extent( mpPointLayer, myFileName ) );
}
 */

QTEST_MAIN( TestColumnCorrectorTool )
#include "tools/moc_columncorrector_test.cxx"

