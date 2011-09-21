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
    void createDisparityMap();
    void combineBandsInTiffTest();

  private: // disabled tests
    void concurTest();
    void simpleFirstTest();
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

int clamp(int value, int vmin, int vmax)
{
    int result;

    if (value < vmin)
        result = vmin;
    else if (value > vmax)
        result = vmax;
    else 
        result = value;

    return result;
}

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


void TestBandAlignerTool::combineBandsInTiffTest()
{
    if (GDALGetDriverCount() == 0)
        GDALAllRegister();  

    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");

    GDALDataset *inputDataset = (GDALDataset*)GDALOpen("J:/tmp/fruit-basket.1024x768.jpg", GA_ReadOnly);

    int mWidth = inputDataset->GetRasterXSize();
    int mHeight = inputDataset->GetRasterYSize();

    GDALDataType mDataType = inputDataset->GetRasterBand(1)->GetRasterDataType();
    
    GDALDataset *outputDataset;
    char **outputMetadata;

    outputDataset = driver->Create("J:/tmp/fruit-basket.1024x768-a.tif", mWidth, mHeight, 1, mDataType, 0);    
    outputMetadata = driver->GetMetadata();    
    copyBand(inputDataset->GetRasterBand(1), outputDataset->GetRasterBand(1), +3, -30);
    outputDataset->SetMetadata(outputMetadata);
    GDALClose(outputDataset);

    outputDataset = driver->Create("J:/tmp/fruit-basket.1024x768-b.tif", mWidth, mHeight, 1, mDataType, 0);    
    outputMetadata = driver->GetMetadata();    
    copyBand(inputDataset->GetRasterBand(2), outputDataset->GetRasterBand(1), 0, 0);
    outputDataset->SetMetadata(outputMetadata);
    GDALClose(outputDataset);

    outputDataset = driver->Create("J:/tmp/fruit-basket.1024x768-c.tif", mWidth, mHeight, 1, mDataType, 0);    
    outputMetadata = driver->GetMetadata();    
    copyBand(inputDataset->GetRasterBand(3), outputDataset->GetRasterBand(1), -5, +25);
    outputDataset->SetMetadata(outputMetadata);
    GDALClose(outputDataset);

    GDALClose(inputDataset);

    __asm int 3; // Hard code a debugger breakpoint in C++
}


QString scale(const QString &value)
{
    qDebug() << "Value on thread " << QThread::currentThread() << " was " << value;

    //int delay = rand() * 250 / RAND_MAX;
    //QThread::sleep(delay);

    return value + ", ";
}

void reduceScale(QString &result, const QString &value)
{
    qDebug() << "Reducing value " << value;
    result = result + value;
}

#if 0

class ImageWorker
{
private:
    QList<QString> m_list;
public:
    ImageWorker() {
        m_list.append("one");
        m_list.append("two");
        m_list.append("three");
        m_list.append("four");
        m_list.append("five");
        m_list.append("six");
        m_list.append("seven");
        m_list.append("eight");
        m_list.append("nine");
        m_list.append("ten");
    }
    ~ImageWorker() {}

    class const_iterator {
    };

/*
    inline const_iterator begin() const { 
        //return reinterpret_cast<Node *>(p.begin());         
        const_iterator i;
        return i;
    }
    inline const_iterator end() const { 
        //return reinterpret_cast<Node *>(p.end());         
        const_iterator i;
        return i;
    }
 */


};


    int m_newX;
    int m_newY;

    QgsRegion m_region1;
    QgsRegion m_region2;
    int m_bits;


    void LoadRegionData(
        GDALRasterBand *refRasterBand, GDALRasterBand srcRasterBand,
        int newX, int newY, float estX, float estY, int mBlockSize, int mBits) 
    {
        int curPoint[2] = { newX, newY };
        int newPoint[2] = { newX + estX, newY + estY };

        uint max1 = getRegion(refRasterBand, NULL, curPoint, mBlockSize, mBits, m_region1);
        uint max2 = getRegion(srcRasterBand, NULL, newPoint, mBlockSize, mBits, m_region2);
    }

};

std::pair<float,float> DisparityScanCorrelator(DisparityScanWorkItem *workItem)
{

    QgsRegionCorrelationResult corrData = 
        QgsRegionCorrelator::findRegion(
            workItem->m_region1, workItem->m_region2, workItem->m_bits);
      
    
    delete [] region1.data;
    delete [] region2.data;

    float corrX;
    float corrY;

    if (corrData.wasSet)
    {
        corrX = corrData.x; 
        corrY = corrData.y;

        printf("Point (%4.0f,%4.0f):%9.4f,%9.4f, match=%8.5f, SNR=%6.3f, run=%3d (%5.3f,%5.3f)\n",
            point[0], point[1], corrX, corrY, corrData.match, corrData.snr, runTimeMs,
            (float)est.real(), (float)est.imag());
    }
    else
    {
        // Fallback to our original estimate
        corrX = workItem->m_estX;
        corrY = workItem->m_estY;
    }

    // TODO: calculate min,max,mean and std-dev of the values corrX & corrY values


    workItem->m_resultX = corrX;
    workItem->m_resultX = corrY;
}

#endif

class DisparityScanWorkItem
{
private:
    QString m_name;

public:
    DisparityScanWorkItem(QString name) {
        m_name = name;
    }
    ~DisparityScanWorkItem() {
    }

    inline QString name() { return m_name; }

    int width;
    int newX, newY;
    float estX, estY;
    float corrX, corrY;
};

DisparityScanWorkItem *DisparityScanCorrelator(DisparityScanWorkItem *workItem)
{
    Q_CHECK_PTR(workItem);

    qDebug() << "Working on thread " << QThread::currentThread() << " was " << workItem->name();

    workItem->width = 20;
    workItem->newX = 0; 
    workItem->newY = 0;
    workItem->estX = 0;
    workItem->estY = 0;
    workItem->corrX = 5;
    workItem->corrY = 30;

    return workItem;
}

class DisparityScanResult 
{
public: 
    unsigned long count;
    double sumX, sumY;
    double sumsqX, sumsqY;
    float *scanlineX;
    float *scanlineY;

    DisparityScanResult(){ //int width) {
        qDebug() << "Creating result.";
        scanlineX = NULL; //new float[width]; 
        scanlineY = NULL; //new float[width]; 
    }
    ~DisparityScanResult() {
         qDebug() << "Deleting scanlines.";
/*
        if (scanlineX != NULL) {
            delete scanlineX;
            scanlineX = NULL;
        }
        if (scanlineY != NULL) {
            delete scanlineY;
            scanlineY = NULL;
        }
 */
    }

};

void DisparityScanReducer(DisparityScanResult &result, DisparityScanWorkItem *workItem)
{
    qDebug() << "Reducing " << workItem->name();

    if (result.scanlineX == NULL)
        result.scanlineX = new float[workItem->width];
    if (result.scanlineY == NULL)
        result.scanlineY = new float[workItem->width];

    /* Store the values in the scanline buffer */
    result.scanlineX[workItem->newX] = workItem->corrX;
    result.scanlineY[workItem->newX] = workItem->corrY;

    /* Update statistics counters */
    result.count += 1;
    result.sumX += workItem->corrX;
    result.sumY += workItem->corrY;
    result.sumsqX += workItem->corrX * workItem->corrX;
    result.sumsqY += workItem->corrY * workItem->corrY;
}

void TestBandAlignerTool::concurTest()
{
    QList<QString> list;    
    list.append("one");
    list.append("two");
    list.append("three");
    list.append("four");
    list.append("five");
    list.append("six");
    list.append("seven");
    list.append("eight");
    list.append("nine");
    list.append("ten");

    using namespace QtConcurrent;
    
    //blockingMappedReduced(list, scale, reduceScale);

    QList<DisparityScanWorkItem *> list1;
    list1.append(new DisparityScanWorkItem("first"));
    list1.append(new DisparityScanWorkItem("second"));
    list1.append(new DisparityScanWorkItem("third"));
    list1.append(new DisparityScanWorkItem("forth"));
    list1.append(new DisparityScanWorkItem("fith"));
    list1.append(new DisparityScanWorkItem("sixth"));
    list1.append(new DisparityScanWorkItem("seven"));
    list1.append(new DisparityScanWorkItem("eight"));
    list1.append(new DisparityScanWorkItem("nine"));
    list1.append(new DisparityScanWorkItem("ten"));

    DisparityScanResult r = blockingMappedReduced(list1, DisparityScanCorrelator, DisparityScanReducer);


    qDebug() << "Result: mean = " << r.sumX/r.count << "," << r.sumY/r.count;
}

#if 0
void TestBandAlignerTool::combineBandsInTiffTest()
{
    //__asm int 3; // Hard code a debugger breakpoint in C++

    QStringList inputPaths;
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C00_F03_MSSK14K_0.tif");
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C01_F03_MSSK14K_0.tif");
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C02_F03_MSSK14K_0.tif");

    QString outputPath = "J:/tmp/test4a.tif";

    if (GDALGetDriverCount() == 0)
        GDALAllRegister();  

    QList<GDALDataset*> inputDataSets;        
    inputDataSets.append((GDALDataset*)GDALOpen(inputPaths[0].toAscii().data(), GA_ReadOnly));
    inputDataSets.append((GDALDataset*)GDALOpen(inputPaths[1].toAscii().data(), GA_ReadOnly));
    inputDataSets.append((GDALDataset*)GDALOpen(inputPaths[2].toAscii().data(), GA_ReadOnly));

    int mWidth = inputDataSets[0]->GetRasterXSize();
    int mHeight = inputDataSets[0]->GetRasterYSize();
    GDALDataType mDataType = inputDataSets[0]->GetRasterBand(1)->GetRasterDataType();
    
    GDALDriver *driver = inputDataSets[0]->GetDriver();    
    GDALDataset *outputDataset = driver->Create(outputPath.toAscii().data(), mWidth, mHeight, inputDataSets.size(), mDataType, 0);    
    char **outputMetadata = driver->GetMetadata();    

    for (int b = 0; b < inputDataSets.size(); b++) 
    {        
        copyBand(inputDataSets[b], 1, outputDataset, 1+b, 0);
    }
    
    outputDataset->SetMetadata(outputMetadata);

    GDALClose(outputDataset);
    
    for (int i = 0; i < inputDataSets.size(); ++i) 
    {
        GDALClose(inputDataSets[i]);
    }
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

unsigned int getRegion(GDALRasterBand* rasterBand, QMutex *mutex, float point[], int dimension, int bits, QgsRegion &region)
{
    Q_CHECK_PTR(rasterBand);

    int x = int(point[0] + 0.5);
    int y = int(point[1] + 0.5);

    region.width = dimension;
    region.height = dimension;
    region.data = new uint[region.width * region.height];

    int xoff = x - region.width/2;
    int yoff = y - region.height/2;
    
    //xoff = clamp(xoff, 0, rasterBand->GetXSize() - region.width/2);
    //yoff = clamp(yoff, 0, rasterBand->GetYSize() - region.height/2);

    //if (mutex) mutex->lock();
   
    rasterBand->RasterIO(GF_Read, xoff, yoff, region.width, region.height, region.data, region.width, region.height, GDT_UInt32, 0, 0);
    
    //if (mutex) mutex->unlock();

    uint max = 0;
    
    for(int i = 0; i < region.width * region.height; i++)
    {
        uint value = region.data[i];

        if (max < value)
            max = value;

        region.data[i] = bits - value;
    }

    return max;
}

QgsComplex findPoint(float point[], QgsComplex estimate, int blockSize, int bits,
                     GDALRasterBand *refRasterBand, 
                     GDALRasterBand *srcRasterBand)
{
    Q_CHECK_PTR(point);
    Q_CHECK_PTR(refRasterBand);
    Q_CHECK_PTR(srcRasterBand);
    Q_ASSERT(blockSize % 2 == 1);

    float newPoint[2] = {
        point[0] + estimate.real(),
        point[1] + estimate.imag(),
    };

    QgsRegion region1, region2;
    
    getRegion(refRasterBand, NULL, point, blockSize, bits, region1);
    getRegion(srcRasterBand, NULL, newPoint, blockSize, bits, region2);

    QgsRegionCorrelationResult corrData = QgsRegionCorrelator::findRegion(region1, region2, bits);

    delete [] region1.data;
    delete [] region2.data;

    double corrX = estimate.real();
    double corrY = estimate.imag();

    if(corrData.wasSet)
    {
        corrX = corrData.x;
        corrY = corrData.y;

        //cout<<"Point found("<<point[0]<<" "<<point[1]<<"): "<<corrX<<", "<<corrY<<endl;
        //cout<<"match ("<<point[0]<<", "<<point[1]<<"): "<<corrData.match<<endl;
        //cout<<"SNR ("<<point[0]<<", "<<point[1]<<") = "<<corrData.snr<<endl;
        
        /*
        if(corrData.snr > -24 && corrData.snr < -27)
        {      
            return QgsComplex();
        }
        */
    }
    else
    {
        //cout<<"This point could not be found in the other band"<<endl;
    }

    newPoint[0] = point[0] + corrX;
    newPoint[1] = point[1] + corrY;
    return QgsComplex(newPoint[0] - point[0], newPoint[1] - point[1]);
}

#ifdef round
#undef round
#endif
static inline float round(float value) { return value + 0.5f; }

#define BLOCK_SIZE 201
//#define D_BITS 255
#define D_BITS ((1<<12)-1)

typedef
struct _statistics {
    unsigned long Count;
    double Sum;
    double SumSq;
    double Min;
    double Max;
    double Mean;
    double Variance;
    double StdDeviation;

public:
    void InitialiseState() 
    {
        Count = 0;
        Sum = 0.0;
        SumSq = 0.0;
        Min = DBL_MAX;
        Max = -DBL_MAX;
        Mean = 0.0;
        Variance = 0.0;
        StdDeviation = 0.0;
    }

    void UpdateState(double value) 
    { 
        ++Count;
        Sum += value;
        SumSq += value * value;
        if (Min > value)
            Min = value;
        if (Max < value)
            Max = value;
    }

    void FinaliseState()
    {
        Mean = Sum / Count;
        Variance = (SumSq / Count) - Mean;
        StdDeviation = sqrt(Variance);
    }

} StatisticsState;

GDALDataset *scanDisparityMap(GDALRasterBand *refRasterBand, 
                              GDALRasterBand *srcRasterBand, 
                              QString disparityPath, 
                              StatisticsState stats[], 
                              int mBlockSize = BLOCK_SIZE, int mBits = D_BITS)
{
    CPLErr err;

    Q_CHECK_PTR(refRasterBand);
    Q_CHECK_PTR(srcRasterBand);
    Q_CHECK_PTR(stats);

    int srcWidth = srcRasterBand->GetXSize();
    int srcHeight = srcRasterBand->GetYSize();

    int mWidth = refRasterBand->GetXSize();
    int mHeight = refRasterBand->GetYSize();

    Q_ASSERT(mWidth > 0);
    Q_ASSERT(mHeight > 0);
    Q_ASSERT(mBlockSize > 0);
    Q_ASSERT(mBlockSize < mWidth);
    Q_ASSERT(mBlockSize < mHeight);
    Q_ASSERT(mBlockSize % 2 == 1);

    int xStepCount = ceil(mWidth / double(mBlockSize));
    int yStepCount = ceil(mHeight / double(mBlockSize));

    double xStepSize = mWidth / double(xStepCount);
    double yStepSize = mHeight / double(yStepCount);

    FILE *f = fopen("C:/tmp/log-scan", "wb");
    if (f == NULL) f = stdout;

    // Remove old file
    unlink(disparityPath.toAscii().data());

    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");

    // Create the new disparity file using the calculated StepCount as width and height
    GDALDataset *disparityDataSet = driver->Create(disparityPath.toAscii().data(), xStepCount, yStepCount, 1, GDT_CFloat32, 0);
    Q_CHECK_PTR(disparityDataSet);
    GDALRasterBand *disparityBand = disparityDataSet->GetRasterBand(1);

    float *scanline = (float *)VSIMalloc3(xStepCount, 2, sizeof(float));

    QgsRegion region1, region2;
    region1.data = (uint *)VSIMalloc3(mBlockSize, mBlockSize, sizeof(uint));
    region2.data = (uint *)VSIMalloc3(mBlockSize, mBlockSize, sizeof(uint));
    
    /* Initialise the statistics state variables */
    stats[0].InitialiseState();
    stats[1].InitialiseState();

    QTime profileScanlineTime;
    QTime profileStepTime;

    for (int yStep = 0; yStep < yStepCount; ++yStep)
    {
        fprintf(f, "Step %-3d: ", yStep);
        profileScanlineTime.start();

        int y = round((yStep + 0.5) * yStepSize);

        int yA = round((yStep + 0.0) * yStepSize);
        int yB = round((yStep + 1.0) * yStepSize);
        region1.height = region2.height = yB - yA;

        // TODO: fill with estimated values (or zero if not known)
        //err = disparityBand->RasterIO(GF_Read, 0, yStep, xStepCount, 1, scanline, xStepCount, 1, GDT_CFloat32, 0, 0);
        memset(scanline, 0, xStepCount * 2 * sizeof(float));

        fprintf(f, " %d", profileScanlineTime.elapsed());
        profileScanlineTime.start();

        for (int xStep = 0; xStep < xStepCount; ++xStep)
        {
            profileStepTime.start();

            int x = round((xStep + 0.5) * xStepSize); // center of block

            int xA = round((xStep + 0.0) * xStepSize);
            int xB = round((xStep + 1.0) * xStepSize);
            region1.width = region2.width = xB - xA;

            const int idx = 2 * xStep;
            float estX = 0.0; scanline[idx+0];
            float estY = 0.0; scanline[idx+1];

            // Take into account the height and width of the source image. 
            int x1 = xA + estX;
            int y1 = yA + estY;
            int x2 = clamp(x1, 0, srcWidth);
            int y2 = clamp(y1, 0, srcHeight);
            int region2_width = std::min(region2.width, srcWidth - x2);
            int region2_height = std::min(region2.height, srcHeight - y2);
            uint *region2_data = region2.data;
            // Our block can be over the edge of the source image.
            if (x1 != x2 || region2_width != region2.width ||
                y1 != y2 || region2_height != region2.height) 
            {
                int xoff = x2 - x1;
                int yoff = y2 - y1;
                int off_idx = xoff + yoff * region2.width;                
                region2_data = &region2.data[off_idx];
                region2_width -= xoff;
                region2_height -= yoff;
                memset(region2.data, 0, region2.width * region2.height * sizeof(region2.data[0]));
            }

            if (xStep == 5) fprintf(f, " [ %d", profileStepTime.elapsed());
            //profileStepTime.start();
                        
            err = refRasterBand->RasterIO(GF_Read, xA, yA, region1.width, region1.height, region1.data, region1.width, region1.height, GDT_UInt32, 0, 0);
            if (err) {
                fprintf(f, "RasterIO(%4d,%4d) error: %d - %s\n", xA,yA, err, CPLGetLastErrorMsg());
            }

            if (xStep == 5) fprintf(f, " %d", profileStepTime.elapsed());
            //profileStepTime.start();

            err = srcRasterBand->RasterIO(GF_Read, x2, y2, region2_width, region2_height, region2_data, region2.width, region2_height, GDT_UInt32, 0, 0);
            if (err) {
                fprintf(f, "RasterIO(%4d,%4d)(%3d,%3d)(%3d,%3d) error: %d - %s\n", 
                    x2,y2, 
                    region2_width, region2_height, 
                    region2.width, region2.height, 
                    err, CPLGetLastErrorMsg());
            }

            if (xStep == 5) fprintf(f, "_%d", profileStepTime.elapsed());
            //profileStepTime.start();

            for (int i = 0; i < region1.width * region2.height; ++i)
            {
                region1.data[i] = mBits - region1.data[i];
                region2.data[i] = mBits - region2.data[i];
            }

            if (xStep == 5) fprintf(f, " %d", profileStepTime.elapsed());

            QTime regionCorrelatorTime;
            regionCorrelatorTime.start();

#if 0
            QgsRegionCorrelationResult corrData(false);
#else
            QgsRegionCorrelationResult corrData = 
                QgsRegionCorrelator::findRegion(region1, region2, mBits);  // HOTSPOT
#endif

            int runTimeMs = regionCorrelatorTime.elapsed();
            if (xStep == 5) fprintf(f, " (%d)", runTimeMs);
            //profileStepTime.start();
            
            float corrX;
            float corrY;

            if (corrData.wasSet)
            {
                corrX = corrData.x; 
                corrY = corrData.y;

                //cout<<"Point found("<<point[0]<<" "<<point[1]<<"): "<<corrX<<", "<<corrY<<endl;
                //cout<<"      match("<<point[0]<<","<<point[1]<<"): "<<corrData.match<<endl;
                //cout<<"        SNR("<<point[0]<<","<<point[1]<<") = "<<corrData.snr<<endl;

                //cout << "Point found(" << point[0] << " " << point[1] <<"): "
                //      << corrX << ", " << corrY
                //      << " match = " << corrData.match
                //      << " SNR = " << corrData.snr 
                //      << endl;

                fprintf(stdout, "Point (%4d,%4d):%9.4f,%9.4f, match=%8.5f, SNR=%6.3f, run=%3d (%5.3f,%5.3f)\n",
                            x, y, corrX, corrY, corrData.match, corrData.snr, runTimeMs, estX, estY);

            }
            else
            {
                corrX = estX;
                corrY = estY;

                //cout<<"This point could not be found in the other band"<<endl;

                //fprintf(f, "Point (%4d,%4d) could not be found, run=%3d, est=(%5.3f,%5.3f)\n",
                //    x, y, runTimeMs, estX, estY);
            }

            /* Save values in output buffer */
            scanline[idx+0] = corrX;
            scanline[idx+1] = corrY;

            /* Update statistics variables */
            stats[0].UpdateState(corrX);
            stats[1].UpdateState(corrY);

            if (xStep == 5) fprintf(f, " %d ]", profileStepTime.elapsed());
        }

        fprintf(f, " x %d steps = %d ms", xStepCount, profileScanlineTime.elapsed());
        profileScanlineTime.start();

        // Write the scanline buffer to the image
        err = disparityBand->RasterIO(GF_Write, 0, yStep, xStepCount, 1, scanline, xStepCount, 1, GDT_CFloat32, 0, 0);
        if (err) {
            fprintf(f, "RasterIO(WRITE)(%4d,%4d)(%3d,%3d) error: %d - %s\n", 
                0, yStep, 
                xStepCount, 1, 
                err, CPLGetLastErrorMsg());
        }
        
        fprintf(f, " EXIT %d\n", profileScanlineTime.elapsed());
        //fprintf(f, "Step %-3d completed in %4d ms\n", yStep, profileScanlineTime.elapsed());
    }

    VSIFree(region2.data);
    VSIFree(region1.data);    

    VSIFree(scanline);

    /* Update the statistical mean and std deviation values */
    stats[0].FinaliseState();
    stats[1].FinaliseState();

    if (f != stdout)
        fclose(f);

    return disparityDataSet;
    //GDALClose(disparityDataSet);
}

bool pointIsBad(double value, double mean, double deviation, double sigmaWeight)
{
    return value > (mean + (deviation*sigmaWeight)) 
        || value < (mean - (deviation*sigmaWeight));
}

float CalculateNineCellAverageWithCheck(
        float *scanline[], int entry,
        int xStep, int xStepLow, int xStepHigh,
        int yStep, int yStepLow, int yStepHigh,
        double mean, double stdDeviation, double sigmaWeight)
{
#define IDX(i) (2*(i) + entry) // scanline contain complex float values
#define ISBAD(v) pointIsBad((v), mean, stdDeviation, sigmaWeight)
#define APPLY_AVG_CHECK(v) do { \
    float __v = (v); \
    if (!ISBAD(__v)) { \
        avgSum += (__v); \
        avgCounter++; \
    } \
} while(0)

    if (!ISBAD(scanline[1][IDX(xStep)])) {
        return scanline[1][IDX(xStep)];
    }

    int avgCounter = 0;
    double avgSum = 0.0;

    if (yStep > yStepLow) {
        if (xStep > xStepLow) {             
            APPLY_AVG_CHECK(scanline[0][IDX( xStep-1 )]); // above left   
        }        
        APPLY_AVG_CHECK(scanline[0][IDX( xStep )]);       // above middle
        if (xStep < xStepHigh) {
            APPLY_AVG_CHECK(scanline[0][IDX( xStep+1 )]); // above right
        }
    }
    if (xStep > xStepLow) {
        APPLY_AVG_CHECK(scanline[1][IDX( xStep-1 )]);     // middle left
    }
    if (xStep < xStepHigh) {
        APPLY_AVG_CHECK(scanline[1][IDX( xStep+1 )]);     // middle right
    }
    if (yStep < yStepHigh) {
        if (xStep > xStepLow) {             
            APPLY_AVG_CHECK(scanline[2][IDX( xStep-1 )]); // below left
        }        
        APPLY_AVG_CHECK(scanline[2][IDX( xStep )]);       // below middle
        if (xStep < xStepHigh) {
            
            APPLY_AVG_CHECK(scanline[2][IDX( xStep+1 )]); // below right
        }
    }

    return avgSum / avgCounter;

#undef APPLY_AVG_CHECK
#undef ISBAD
#undef IDX
}

void eliminateDisparityMap(GDALRasterBand *disparityBand, StatisticsState stats[], double sigmaWeight = 2.0)
{
    Q_CHECK_PTR(disparityBand);
    Q_CHECK_PTR(stats);

    int xStepCount = disparityBand->GetXSize();
    int yStepCount = disparityBand->GetYSize();

    // Allocate buffer space for the 3 input and 1 output scanline
    float *scanlineA = (float *)malloc(xStepCount * 2 * sizeof(float));
    float *scanlineB = (float *)malloc(xStepCount * 2 * sizeof(float));
    float *scanlineC = (float *)malloc(xStepCount * 2 * sizeof(float));
    float *scanlineO = (float *)malloc(xStepCount * 2 * sizeof(float));

    // Setup the rotating buffer array
    float *scanline[3] = { scanlineA, scanlineB, scanlineC };

    // Clear the first buffer because we start at the top (zero edge extend)
    memset(scanline[0], 0, xStepCount * 2 * sizeof(float));

    // Preload the first scanline 
    disparityBand->RasterIO(GF_Read, 0, 0, xStepCount, 1, scanline[1], xStepCount, 1, GDT_CFloat32, 0, 0);

    // Correct all points lying outside the mean/stdev range
    for (int yStep = 0; yStep < yStepCount; ++yStep) {

        // Load the next scanline from disk
        if (yStep < yStepCount-1) {
            disparityBand->RasterIO(GF_Read, 0, yStep+1, xStepCount, 1, scanline[2], xStepCount, 1, GDT_CFloat32, 0, 0);
        } else {
            memset(scanline[2], 0, xStepCount * 2 * sizeof(float)); // (zero edge extend)
        }

        // Perform the processing on the 3 scanline
        for (int xStep = 0; xStep < xStepCount; ++xStep) 
        {
            scanlineO[2*xStep+0] = CalculateNineCellAverageWithCheck(scanline, 0, 
                xStep, 0, xStepCount-1, yStep, 0, yStepCount-1,
                stats[0].Mean, stats[0].StdDeviation, sigmaWeight);

            scanlineO[2*xStep+1] = CalculateNineCellAverageWithCheck(scanline, 1, 
                xStep, 0, xStepCount-1, yStep, 0, yStepCount-1,
                stats[1].Mean, stats[1].StdDeviation, sigmaWeight);
        }

        // Write the result out to disk
        disparityBand->RasterIO(GF_Write, 0, yStep, xStepCount, 1, scanlineO, xStepCount, 1, GDT_CFloat32, 0, 0);

        // Rotate the scanline 
        float *tmp = scanline[0];
        scanline[0] = scanline[1];
        scanline[1] = scanline[2];
        scanline[2] = tmp;
    }

    // Free the buffer space after use
    free(scanlineO);
    free(scanlineC);
    free(scanlineB);
    free(scanlineA);
}

void estimateDisparityMap(
        GDALRasterBand *disparityBand,
        GDALRasterBand *disparityMapX,
        GDALRasterBand *disparityMapY,
        int mBlockSize = BLOCK_SIZE)
{
    Q_CHECK_PTR(disparityBand);
    Q_CHECK_PTR(disparityMapX);
    Q_CHECK_PTR(disparityMapY);

    Q_ASSERT(mBlockSize > 0);
    Q_ASSERT(mBlockSize % 2 == 1);

    Q_ASSERT(disparityBand->GetRasterDataType() == GDT_CFloat32);

    Q_ASSERT(disparityMapX->GetXSize() == disparityMapY->GetXSize());
    Q_ASSERT(disparityMapX->GetYSize() == disparityMapY->GetYSize());

    int mWidth = disparityMapX->GetXSize();
    int mHeight = disparityMapX->GetYSize();

    Q_ASSERT(mWidth > 0);
    Q_ASSERT(mHeight > 0);
    Q_ASSERT(mBlockSize < mWidth);
    Q_ASSERT(mBlockSize < mHeight);

    int xStepCount = 1 + disparityBand->GetXSize();
    int yStepCount = 1 + disparityBand->GetYSize();

    double xStepSize = mWidth / double(xStepCount);
    double yStepSize = mHeight / double(yStepCount);

    Q_CHECK_PTR(xStepSize <= mBlockSize);
    Q_CHECK_PTR(yStepSize <= mBlockSize);

    disparityMapX->SetNoDataValue(0.0);
    disparityMapY->SetNoDataValue(0.0);

    const bool constantEdgeExtend = true; // or false for zero edge-extend

    FILE *f = fopen("C:/tmp/log.txt", "wb");
    if (f == NULL) f = stdout;

    fprintf(f, "Step Count = (%d, %d), Step Size = (%f, %f)\n", xStepCount, yStepCount, xStepSize, yStepSize);

    float *disparityBufferX = (float *)malloc(mBlockSize*mBlockSize * sizeof(float)); 
    float *disparityBufferY = (float *)malloc(mBlockSize*mBlockSize * sizeof(float)); 

    float *disparityBuffer1 = (float *)malloc((2 + xStepCount) * 2 * sizeof(float *));
    float *disparityBuffer2 = (float *)malloc((2 + xStepCount) * 2 * sizeof(float *));    

    float *disparityBuffer[2];    

    if (constantEdgeExtend) {
        disparityBuffer[0] = disparityBuffer1;
        disparityBuffer[1] = disparityBuffer1;
    } else { // zero edge extend
        memset(disparityBuffer2, 0, (2 + xStepCount) * 2 * sizeof(float *));
        disparityBuffer[0] = disparityBuffer2;
        disparityBuffer[1] = disparityBuffer1;
    }

    int ySubStartNext = 0;

    QTime profileRowTimer;
    QTime profileTimer;

    for (int yStep = 0; yStep <= yStepCount; ++yStep)
    {
        profileRowTimer.start();

        int yA = round((yStep - 0.5) * yStepSize); // above
        int yY = round((yStep + 0.0) * yStepSize); // middle
        int yB = round((yStep + 0.5) * yStepSize); // below

        int ySubSize = (0 < yStep && yStep < yStepCount) ? (yB - yA) : (yStep == 0) ? yB - yY : yY - yA;
        int ySubStart = (yStep == 0) ? yY : yA;

        Q_ASSERT(0 < ySubSize && ySubSize <= mBlockSize);
        Q_ASSERT(0 <= ySubStart && (ySubStart + ySubSize) <= mHeight);

        // Verify that the block follow one another nicely 
        Q_ASSERT(ySubStart == ySubStartNext);
        ySubStartNext = ySubStart + ySubSize;

        // Load data from disk
        if (yStep < yStepCount) {
            disparityBand->RasterIO(GF_Read, 0, yStep, xStepCount-1, 1, &disparityBuffer[1][2], xStepCount-1, 1, GDT_CFloat32, 0, 0);

            if (constantEdgeExtend) {
                disparityBuffer[1][0] = disparityBuffer[1][2];
                disparityBuffer[1][1] = disparityBuffer[1][3];
                const int last = (1 + xStepCount) * 2;
                disparityBuffer[1][last+0] = disparityBuffer[1][last-2];
                disparityBuffer[1][last+1] = disparityBuffer[1][last-1];
            } else { // zero edge-extend
                disparityBuffer[1][0] = 0.0f;
                disparityBuffer[1][1] = 0.0f;
                const int last = (1 + xStepCount) * 2;
                disparityBuffer[1][last+0] = 0.0f;
                disparityBuffer[1][last+1] = 0.0f;
            }        
        } else if (constantEdgeExtend) {            
            disparityBuffer[1] = disparityBuffer[0];
        } else { // zero edge-extend
            memset(disparityBuffer[1], 0, (2 + xStepCount) * 2 * sizeof(float *));
        }
        
        int xSubStartNext = 0;
        
        for (int xStep = 0; xStep <= xStepCount; ++xStep)
        {
            profileTimer.start();

            int xA = round((xStep - 0.5) * xStepSize); // left
            int xX = round((xStep + 0.0) * xStepSize); // middle
            int xB = round((xStep + 0.5) * xStepSize); // right

            int xSubSize = (0 < xStep && xStep < xStepCount) ? (xB - xA) : (xStep == 0) ? xB - xX : xX - xA;
            int xSubStart = (xStep == 0) ? xX : xA;

            Q_ASSERT(0 < xSubSize && xSubSize <= mBlockSize);
            Q_ASSERT(0 <= xSubStart && (xSubStart + xSubSize) <= mWidth);

            // Verify that the block follow one another without gaps or overlaps 
            Q_ASSERT(xSubStart == xSubStartNext);
            xSubStartNext = xSubStart + xSubSize;

            const int idx1 = (1 + xStep - 1) * 2;
            const int idx2 = (1 + xStep - 0) * 2;
            
            float *p11 = &disparityBuffer[0][idx1]; // col = 1, row = 1
            float *p12 = &disparityBuffer[1][idx1]; // col = 1, row = 2
            float *p21 = &disparityBuffer[0][idx2]; // col = 2, row = 1
            float *p22 = &disparityBuffer[1][idx2]; // col = 2, row = 2

            for(int ySub = 0; ySub < ySubSize; ySub++)
            {
                //int y = ySubStart + ySub;

                for(int xSub = 0; xSub < xSubSize; xSub++)
                {
                    //int x = xSubStart + xSub;
                        
                    QgsComplex p1(p11[0], p11[1]);
                    QgsComplex p2(p21[0], p21[1]);
                    QgsComplex p3(p12[0], p12[1]);
                    QgsComplex p4(p22[0], p22[1]);

                    QgsComplex top = (p2*double(xSub) + p1*double(xSubSize - xSub)) / double(xSubSize);
                    QgsComplex bottom = (p4*double(xSub) + p3*double(xSubSize - xSub)) / double(xSubSize);
                    QgsComplex middle = (top*double(ySubSize - ySub) + bottom*double(ySub)) / double(ySubSize);

                    int idx = xSub + ySub * xSubSize;
                    
                    disparityBufferX[idx] = middle.real();
                    disparityBufferY[idx] = middle.imag();
                }
            }
            
            fprintf(f, "[%3d ms] ", profileTimer.elapsed());   

            fprintf(f, "Step: (%2d,%2d), (%4d,%4d), (%4d,%4d), (%4d,%4d) W (%4d,%4d)-(%3d,%3d)-(%4d,%4d) B 0x%08X 0x%08X P 0x%08X 0x%08X 0x%08X 0x%08X", 
                xStep,yStep, xX,yY, xA,yA, xB,yB, 
                xSubStart, ySubStart, xSubSize, ySubSize, xSubStart + xSubSize - 1, ySubStart + ySubSize - 1,
                (int)disparityBuffer[0], (int)disparityBuffer[1],  // check that they are swopped
                (int)p11, (int)p12, (int)p21, (int)p22); // verify duplicate pointers for first/last row/columns

            //fprintf(f, "disparityMap->RasterIO(GF_Write, %4d, %4d, %4d, %4d, disparityBuffer, %4d, %4d, GDT_Float32, 0, 0)\n", 
            //    xSubStart, ySubStart, xSubSize, ySubSize, xSubSize, ySubSize);

            profileTimer.start();

            Q_ASSERT(0 <= xSubStart && xSubStart < mWidth);
            Q_ASSERT(0 <= ySubStart && ySubStart < mHeight);
            Q_ASSERT((xSubStart + xSubSize) <= mWidth);
            Q_ASSERT((ySubStart + ySubSize) <= mHeight);

            CPLErr err;
            err = disparityMapX->RasterIO(GF_Write, xSubStart, ySubStart, xSubSize, ySubSize, disparityBufferX, xSubSize, ySubSize, GDT_Float32, 0, 0);             
            err = disparityMapY->RasterIO(GF_Write, xSubStart, ySubStart, xSubSize, ySubSize, disparityBufferY, xSubSize, ySubSize, GDT_Float32, 0, 0);

            fprintf(f, " EXIT %d\n", profileTimer.elapsed());  
        }

        if (yStep > 0) {
            // Swap line buffers
            float *tmp = disparityBuffer[0];
            disparityBuffer[0] = disparityBuffer[1];
            disparityBuffer[1] = tmp;
        } else {
            // Ensure the two buffers is in the correct order
            disparityBuffer[0] = disparityBuffer1;
            disparityBuffer[1] = disparityBuffer2;
        }

        fprintf(f, "*** Row completed in %d ms\n", profileRowTimer.elapsed());  
    }

    if (f != stdout)
        fclose(f);

    free(disparityBuffer2); 
    free(disparityBuffer1); 

    free(disparityBufferY);
    free(disparityBufferX);

    //printf("done");
#if 0
    int inWidth = disparitySubsetX->GetXSize();
    int inHeight = disparitySubsetX->GetYSize();

    Q_ASSERT(disparityMapX->GetXSize() == disparityMapY->GetXSize());
    Q_ASSERT(disparityMapX->GetYSize() == disparityMapY->GetYSize());

    int mWidth = disparityMapX->GetXSize();
    int mHeight = disparityMapY->GetYSize();

    int xSteps = ceil(mWidth / double(mBlockSize));
    int ySteps = ceil(mHeight / double(mBlockSize));

    float xDiff = (xSteps * mBlockSize - mWidth) / float(xSteps - 1);
    float yDiff = (ySteps * mBlockSize - mHeight) / float(ySteps - 1);
  
    int blockB = floor((mBlockSize + 1) / 2.0);
      
    float *disparitySubsetX_y1 = (float *)malloc(inWidth * sizeof(float *)); 
    float *disparitySubsetY_y1 = (float *)malloc(inWidth * sizeof(float *)); 
    float *disparitySubsetX_y2 = (float *)malloc(inWidth * sizeof(float *)); 
    float *disparitySubsetY_y2 = (float *)malloc(inWidth * sizeof(float *)); 
    
    float *disparityBufferX = (float *)malloc(mBlockSize*mBlockSize * sizeof(float)); 
    float *disparityBufferY = (float *)malloc(mBlockSize*mBlockSize * sizeof(float)); 

    float blockCount = 0.0; 

    int xTo = xSteps+1;
    int yTo = ySteps+1;

    for(int y = 0; y < yTo; y++)
    {
        int y1 = floor((y-0.5)*mBlockSize - fabs((float)(y-1))*yDiff);
        int y2 = floor((y+0.5)*mBlockSize - y*yDiff);

        if(!(y1 > 0 && y1 <= mHeight-blockB))
        {
          y1 = y2;
        }
        if(!(y2 > 0 && y2 <= mHeight-blockB))
        {
          y2 = y1;
        }

        int newY;
        int startY;
        if(y2-y1)
        {
          newY = y2 - y1;
          startY = y1;
        }
        else
        {
          newY = min(y1, mHeight-y1);
          startY = (y1 > blockB) ? y1 : 0;
        }

        disparitySubsetX->RasterIO(GF_Read, 0, y1, inWidth, 1, disparitySubsetX_y1, inWidth, 1, GDT_Float32, 0, 0);
        disparitySubsetY->RasterIO(GF_Read, 0, y1, inWidth, 1, disparitySubsetY_y1, inWidth, 1, GDT_Float32, 0, 0);
        
        disparitySubsetX->RasterIO(GF_Read, 0, y2, inWidth, 1, disparitySubsetX_y2, inWidth, 1, GDT_Float32, 0, 0);
        disparitySubsetY->RasterIO(GF_Read, 0, y2, inWidth, 1, disparitySubsetY_y2, inWidth, 1, GDT_Float32, 0, 0);        

        for (int x = 0; x < xTo; x++)
        {
            int x1 = floor((x-0.5)*mBlockSize - fabs((float)(x-1))*xDiff);
            int x2 = floor((x+0.5)*mBlockSize - x*xDiff);

            if (!(0 < x1 && x1 <= mWidth-blockB))
            {
                x1 = x2;
            }
            if (!(0 < x2 && x2 <= mWidth-blockB))
            {
                x2 = x1;
            }

            int newX;        
            int startX;
            if(x2 - x1)
            {
                newX = x2 - x1;
                startX = x1;
            }
            else
            {
                newX = min(x1, mWidth - x1);
                startX = (x1 > blockB) ? x1 : 0;
            }

            Q_ASSERT(x1 >= 0 && x2 >= 0);
            Q_ASSERT(y1 >= 0 && x2 >= 0);            

            // -------------------------------

            QgsComplex p1;
            QgsComplex p2;
            QgsComplex p3;
            QgsComplex p4;

            if(y1 < mHeight && x1 < mWidth)
            {
                p1 = disparitySubsetX_y1[x1];
            }

            if(y1 < mHeight && x2 < mWidth)
            {
                p2 = disparitySubsetX_y1[x2];
            }

            if(y2 < mHeight && x1 < mWidth)
            {
                p3 = disparitySubsetX_y2[x1];
            }

            if(y2 < mHeight && x2 < mWidth)
            {
                p4 = disparitySubsetX_y2[x2];
            }

            // -------------------------------

            // func( int newX, int newY, int startX, int start Y, width, height, p1, p2, p3, p4)

            for(int ySub = 0; ySub < newY; ySub++)
            {
                int newYSub = startY + ySub;                  

                for(int xSub = 0; xSub < newX; xSub++)
                {
                    int newXSub = startX + xSub;

                    // ---------------------

                    QgsComplex top = (p2*double(xSub) + p1*double(newX-xSub)) / double(newX);
                    QgsComplex bottom = (p4*double(xSub) + p3*double(newX-xSub)) / double(newX);
                    QgsComplex middle = (top*double(newY - ySub) + bottom*double(ySub)) / double(newY);

                    //cout<<"Real2: "<<newY<<" "<<y1<<" "<<y2<<" "<<p1.imag()<<" "<<middle.imag()<<endl;
                    int index = newYSub*mWidth + newXSub;

                    //mDisparityReal[index] = middle.real();
                    //mDisparityImag[index] = middle.imag();

                    int idx = xSub + ySub*newX;
                    disparityBufferX[idx] = middle.real();
                    disparityBufferY[idx] = middle.imag();

                    //printf("mDisparity[%4d,%4d] = (%.3f, %.3f) %d\n", 
                    //    newXSub, newYSub, (float)middle.real(), (float)middle.imag(), idx);  


                    // ---------------------
                }
            }

            //printf("=======================================\n");
            //printf("disparityReal->RasterIO(GF_Write, %4d, %4d, %4d, %4d, disparityBufReal, %4d, %4d, GDT_Float32, 0, 0)", 
            //    startX, startY, newX, newY, newX, newY);

            Q_ASSERT(0 <= startX && startX < mWidth);
            Q_ASSERT(0 <= startY && startY < mHeight);
            Q_ASSERT((startX + newX) <= mWidth);
            Q_ASSERT((startY + newY) <= mHeight);

            disparityMapX->RasterIO(GF_Write, startX, startY, newX, newY, disparityBufferX, newX, newY, GDT_Float32, 0, 0);
            disparityMapY->RasterIO(GF_Write, startX, startX, newX, newY, disparityBufferY, newX, newY, GDT_Float32, 0, 0);

            // -------------------------------
        }
    }

    // Free all the buffer memory 
    free(disparityBufferY);
    free(disparityBufferX);

    free(disparitySubsetY_y2);
    free(disparitySubsetX_y2);
    free(disparitySubsetY_y1);
    free(disparitySubsetX_y1);
#endif
}

template <typename T>
int cubicConvolution(T data[], float shift[])
{
    Q_CHECK_PTR(data);
    Q_CHECK_PTR(shift);

    float xShift = shift[0];
    float xShift2 = xShift*xShift;
    float xShift3 = xShift*xShift2;

    float xData[4] = {
        -(1/2.0)* xShift3 + (2/2.0)*xShift2 - (1/2.0)*xShift,
         (3/2.0)* xShift3 - (5/2.0)*xShift2 + (2/2.0),
        -(3/2.0)* xShift3 + (4/2.0)*xShift2 + (1/2.0)*xShift,
         (1/2.0)* xShift3 - (1/2.0)*xShift2,
    };

    float yShift = shift[1];
    float yShift2 = yShift*yShift;
    float yShift3 = yShift*yShift2;

    float yData[4] = {
        0.5 * (  -yShift3 + 2*yShift2 - yShift),
        0.5 * ( 3*yShift3 - 5*yShift2 + 2 ),
        0.5 * (-3*yShift3 + 4*yShift2 + yShift ),
        0.5 * (   yShift3 -   yShift2),
    };
    
    int result = 0;
    for(int y = 0; y < 4; y++)
    {
        int rowContribution = 0;        
        for(int x = 0; x < 4; x++)
        {
            rowContribution += xData[x] * data[4*y + x];
        }
        result += rowContribution * yData[y];
    }    
    return result;
}

void applyDisparityMap(
        GDALRasterBand *inputRasterData,
        GDALRasterBand *disparityMapX,
        GDALRasterBand *disparityMapY,
        GDALRasterBand *outputRasterData)
{
    Q_CHECK_PTR(inputRasterData);
    Q_CHECK_PTR(disparityMapX);
    Q_CHECK_PTR(disparityMapY);
    Q_CHECK_PTR(outputRasterData);

    int mWidth = inputRasterData->GetXSize();
    int mHeight = inputRasterData->GetYSize();

    float *disparityBufferReal = (float *)malloc(mWidth * sizeof(float));
    float *disparityBufferImag = (float *)malloc(mWidth * sizeof(float));
    float *outputBuffer = (float *) malloc(mWidth * sizeof(float));

    float data[4*4];
    float noDataValue = outputRasterData->GetNoDataValue();

    int inWidth = inputRasterData->GetXSize();
    int inHeight = inputRasterData->GetYSize();

    for(int y = 0; y < mHeight; y++)
    {
        disparityMapX->RasterIO(GF_Read, 0, y, mWidth, 1, disparityBufferReal, mWidth, 1, GDT_Float32, 0, 0);
        disparityMapY->RasterIO(GF_Read, 0, y, mWidth, 1, disparityBufferImag, mWidth, 1, GDT_Float32, 0, 0);

        for(int x = 0; x < mWidth; x++)
        {
            int srcX1 = floor(x - disparityBufferReal[x]);
            int srcY1 = floor(y - disparityBufferImag[x]);

            if ((1 <= srcX1 && srcX1 < inWidth-2) && 
                (1 <= srcY1 && srcY1 < inHeight-2))
            {
                inputRasterData->RasterIO(GF_Read, srcX1-1, srcY1-1, 4, 4, data, 4, 4, GDT_Float32, 0, 0);

                float shift[2] = { 
                    x - disparityBufferReal[x] - srcX1, 
                    y - disparityBufferImag[x] - srcY1,
                };

                float value = cubicConvolution(data, shift);

                //if (value > mBits)
                //    value = noDataValue;
                //else if (value > mMax)
                //    value = noDataValue;

                outputBuffer[x] = value;
            }
            else
            {
                outputBuffer[x] = noDataValue;
            }
        }

        outputRasterData->RasterIO(GF_Write, 0, y, mWidth, 1, outputBuffer, mWidth, 1, GDT_Float32, 0, 0);
    }
}

void TestBandAlignerTool::createDisparityMap()
{
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
}

void TestBandAlignerTool::simpleFirstTest()
{
    //QStringList inputPaths;
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C00_F03_MSSK14K_0.tif");
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C01_F03_MSSK14K_0.tif");
    //inputPaths.append(QString(TEST_DATA_DIR) + "/I0D79/16bit/" + "I0D79_P03_S02_C02_F03_MSSK14K_0.tif");
    //QString outputPath = QDir::tempPath() + "test_bandaligner-result" + ".tif";

    QStringList inputPaths;
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C00_F03_MSSK14K_0.tif");
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C01_F03_MSSK14K_0.tif");
    inputPaths.append("J:/TestData/I0BDA/16bit/I0BDA_P03_S02_C02_F03_MSSK14K_0.tif");

    QString outputPath = "J:/tmp/test3.tif";
    QString dispXPath = "J:/tmp/test3-x.tif";
    QString dispYPath = "J:/tmp/test3-y.tif";

    QgsBandAlignerThread *mThread = new QgsBandAlignerThread(inputPaths, outputPath, dispXPath, dispYPath, 101);
   
    //mThread->run();

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

