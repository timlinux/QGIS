#include "QgsImageAligner.h"

#undef QT_NO_DEBUG
#define QT_NO_EXCEPTIONS 1
#include <QtGlobal>

#include <QDir>
#include <QTime>

/* ************************************************************************* */

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

/* ************************************************************************* */

static int clamp(int value, int vmin, int vmax)
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

/* ************************************************************************* */

static void scanDisparityMap(QgsProgressMonitor &monitor,
    GDALRasterBand *refRasterBand,  GDALRasterBand *srcRasterBand,
    GDALRasterBand *disparityBand,  StatisticsState stats[])
{
    CPLErr err;

    Q_CHECK_PTR(refRasterBand);
    Q_CHECK_PTR(srcRasterBand);
    Q_CHECK_PTR(disparityBand);
    Q_CHECK_PTR(stats);

    int mWidth = refRasterBand->GetXSize();
    int mHeight = refRasterBand->GetYSize();

    // FIXME: 8bit & 16bit is ok, but not 32bit, float and double (and complex!)
    int mBits = 1 << GDALGetDataTypeSize(refRasterBand->GetRasterDataType());

    Q_ASSERT(mWidth > 0);
    Q_ASSERT(mHeight > 0);

    //FILE *f = fopen("C:/tmp/log-scan", "wb");
    //if (f == NULL) f = stdout;

    int xStepCount = disparityBand->GetXSize();
    int yStepCount = disparityBand->GetYSize();

    Q_ASSERT(xStepCount > 0);
    Q_ASSERT(yStepCount > 0);
    Q_ASSERT(xStepCount < mWidth);
    Q_ASSERT(yStepCount < mHeight);

    int srcWidth = srcRasterBand->GetXSize();
    int srcHeight = srcRasterBand->GetYSize();

    double xStepSize = mWidth / double(xStepCount);
    double yStepSize = mHeight / double(yStepCount);

    float *scanline = (float *)VSIMalloc3(xStepCount, 2, sizeof(float));

    QgsRegion region1, region2;
    region1.data = (uint *)VSIMalloc3(ceil(xStepSize), ceil(yStepSize), sizeof(uint));
    region2.data = (uint *)VSIMalloc3(ceil(xStepSize), ceil(yStepSize), sizeof(uint));
    
    /* Initialise the statistics state variables */
    stats[0].InitialiseState();
    stats[1].InitialiseState();

    //QTime profileScanlineTime;
    //QTime profileStepTime;

    double workBase = monitor.GetProgress();
    double workUnit = monitor.GetWorkUnit() / yStepCount;

    for (int yStep = 0; yStep < yStepCount; ++yStep)
    {
        if (monitor.IsCanceled()) 
            break;

        monitor.SetProgress(workBase + workUnit * yStep);
        
        //fprintf(f, "Step %-3d: ", yStep);
        //profileScanlineTime.start();

        int y = round((yStep + 0.5) * yStepSize); // center of block

        int yA = round((yStep + 0.0) * yStepSize);
        int yB = round((yStep + 1.0) * yStepSize);
        region1.height = region2.height = yB - yA;

        // TODO: fill with estimated values (or zero if not known)
        //err = disparityBand->RasterIO(GF_Read, 0, yStep, xStepCount, 1, scanline, xStepCount, 1, GDT_CFloat32, 0, 0);
        memset(scanline, 0, xStepCount * 2 * sizeof(float));

        //fprintf(f, " %d", profileScanlineTime.elapsed());
        //profileScanlineTime.start();

        for (int xStep = 0; xStep < xStepCount; ++xStep)
        {
            if (monitor.IsCanceled()) {
                monitor.SetProgress(workBase + workUnit * (yStep + xStep / (double)xStepCount));
                break;            
            }

            //profileStepTime.start();

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

            //if (xStep == 5) fprintf(f, " [ %d", profileStepTime.elapsed());
            //profileStepTime.start();
                        
            err = refRasterBand->RasterIO(GF_Read, xA, yA, region1.width, region1.height, region1.data, region1.width, region1.height, GDT_UInt32, 0, 0);
            if (err) {
                // TODO: error handling 
                //fprintf(f, "RasterIO(%4d,%4d) error: %d - %s\n", xA,yA, err, CPLGetLastErrorMsg());
            }

            //if (xStep == 5) fprintf(f, " %d", profileStepTime.elapsed());
            //profileStepTime.start();

            err = srcRasterBand->RasterIO(GF_Read, x2, y2, region2_width, region2_height, region2_data, region2.width, region2_height, GDT_UInt32, 0, 0);
            if (err) {
                // TODO: error handling 
                //fprintf(f, "RasterIO(%4d,%4d)(%3d,%3d)(%3d,%3d) error: %d - %s\n", 
                //    x2,y2, 
                //    region2_width, region2_height, 
                //    region2.width, region2.height, 
                //    err, CPLGetLastErrorMsg());
            }

            //if (xStep == 5) fprintf(f, "_%d", profileStepTime.elapsed());

            //QTime regionCorrelatorTime;
            //regionCorrelatorTime.start();

            QgsRegionCorrelationResult corrData = 
                QgsRegionCorrelator::findRegion(region1, region2, mBits);

            //int runTimeMs = regionCorrelatorTime.elapsed();
            //if (xStep == 5) fprintf(f, " (%d)", runTimeMs);
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

                //fprintf(stdout, "Point (%4d,%4d):%9.4f,%9.4f, match=%8.5f, SNR=%6.3f, run=%3d (%5.3f,%5.3f)\n",
                //            x, y, corrX, corrY, corrData.match, corrData.snr, runTimeMs, estX, estY);

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

            //if (xStep == 5) fprintf(f, " %d ]", profileStepTime.elapsed());
        }

        //fprintf(f, " x %d steps = %d ms", xStepCount, profileScanlineTime.elapsed());
        //profileScanlineTime.start();

        // Write the scanline buffer to the image
        err = disparityBand->RasterIO(GF_Write, 0, yStep, xStepCount, 1, scanline, xStepCount, 1, GDT_CFloat32, 0, 0);
        if (err) {
            // TODO: error handling
            //fprintf(f, "RasterIO(WRITE)(%4d,%4d)(%3d,%3d) error: %d - %s\n", 
            //    0, yStep, 
            //    xStepCount, 1, 
            //    err, CPLGetLastErrorMsg());
        }
        
        //fprintf(f, " EXIT %d\n", profileScanlineTime.elapsed());
        //fprintf(f, "Step %-3d completed in %4d ms\n", yStep, profileScanlineTime.elapsed());
    }

    VSIFree(region2.data);
    VSIFree(region1.data);    

    VSIFree(scanline);

    /* Update the statistical mean and std deviation values */
    stats[0].FinaliseState();
    stats[1].FinaliseState();

    //disparityBand->SetStatistics(stats[0].Min, stats[0].Max, stats[0].Mean, stats[0].StdDeviation);

    //if (f != stdout)
    //    fclose(f);
}

/* ************************************************************************* */

static bool pointIsBad(double value, double mean, double deviation, double sigmaWeight)
{
    return value > (mean + (deviation*sigmaWeight)) 
        || value < (mean - (deviation*sigmaWeight));
}

static float CalculateNineCellAverageWithCheck(float *scanline[], int entry,
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

static void eliminateDisparityMap(QgsProgressMonitor &monitor,
    GDALRasterBand *disparityBand, 
    StatisticsState stats[], 
    double sigmaWeight = 2.0)
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

    double workBase = monitor.GetProgress();
    double workUnit = monitor.GetWorkUnit() / yStepCount;

    // Preload the first scanline 
    disparityBand->RasterIO(GF_Read, 0, 0, xStepCount, 1, scanline[1], xStepCount, 1, GDT_CFloat32, 0, 0);

    // Correct all points lying outside the mean/stdev range
    for (int yStep = 0; yStep < yStepCount; ++yStep) 
    {
        if ((yStepCount & 8) == 0) // 8 times rate reduction
            monitor.SetProgress(workBase + workUnit * yStep);

        if (monitor.IsCanceled())
            break;

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

/* ************************************************************************* */

#ifdef round
#undef round
#endif
static inline float round(float value) { return floor(value + 0.5f); }
static inline double round(double value) { return floor(value + 0.5); }
static inline long double round(long double value) { return floor(value + 0.5); }

static void estimateDisparityMap(QgsProgressMonitor &monitor,
    GDALRasterBand *disparityBand, 
    GDALRasterBand *disparityMapX, 
    GDALRasterBand *disparityMapY)
{
    Q_CHECK_PTR(disparityBand);
    Q_CHECK_PTR(disparityMapX);
    Q_CHECK_PTR(disparityMapY);

    Q_ASSERT(mBlockSize > 0);
    Q_ASSERT(mBlockSize % 2 == 1);

    Q_ASSERT(disparityBand->GetRasterDataType() == GDT_CFloat32);

    Q_ASSERT(disparityMapX->GetXSize() == disparityMapY->GetXSize());
    Q_ASSERT(disparityMapX->GetYSize() == disparityMapY->GetYSize());

    const int mWidth = disparityMapX->GetXSize();
    const int mHeight = disparityMapX->GetYSize();

    Q_ASSERT(mWidth > 0);
    Q_ASSERT(mHeight > 0);

    const int xStepCount = disparityBand->GetXSize(); 
    const int yStepCount = disparityBand->GetYSize();

    const double xStepSize = mWidth / double(xStepCount);
    const double yStepSize = mHeight / double(yStepCount);

    disparityMapX->SetNoDataValue(0.0);
    disparityMapY->SetNoDataValue(0.0);

    const bool constantEdgeExtend = true; // or false for zero edge-extend

    //FILE *f = fopen("C:/tmp/log.txt", "wb");
    //if (f == NULL) f = stdout;

    //fprintf(f, "Step Count = (%d, %d), Step Size = (%f, %f)\n", xStepCount, yStepCount, xStepSize, yStepSize);

    const int xBlockSize = ceil(xStepSize);
    const int yBlockSize = ceil(yStepSize);

    float *disparityBufferX = (float *)malloc(xBlockSize*yBlockSize * sizeof(float)); 
    float *disparityBufferY = (float *)malloc(xBlockSize*yBlockSize * sizeof(float)); 

    float *disparityBuffer1 = (float *)malloc((1 + xStepCount + 1) * 2 * sizeof(float *));
    float *disparityBuffer2 = (float *)malloc((1 + xStepCount + 1) * 2 * sizeof(float *));    

    float *disparityBuffer[2];    

    if (constantEdgeExtend) {
        disparityBuffer[0] = disparityBuffer1;
        disparityBuffer[1] = disparityBuffer1;
    } else { // zero edge extend
        memset(disparityBuffer2, 0, (1 + xStepCount + 1) * 2 * sizeof(float *));
        disparityBuffer[0] = disparityBuffer2;
        disparityBuffer[1] = disparityBuffer1;
    }

    double workBase = monitor.GetProgress();
    double workUnit = monitor.GetWorkUnit() / (yStepCount + 1);

    //QTime profileRowTimer;
    //QTime profileTimer;

    int ySubStartNext = 0;

    for (int yStep = 0; yStep <= yStepCount; ++yStep)
    {
        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * yStep);

        //profileRowTimer.start();

        int yA = round((yStep - 0.5) * yStepSize); // above
        int yY = round((yStep + 0.0) * yStepSize); // middle
        int yB = round((yStep + 0.5) * yStepSize); // below

        int ySubSize = (0 < yStep && yStep < yStepCount) ? (yB - yA) : (yStep == 0) ? yB - yY : yY - yA;
        int ySubStart = (yStep == 0) ? yY : yA;

        Q_ASSERT(0 < ySubSize && ySubSize <= yBlockSize);
        Q_ASSERT(0 <= ySubStart && (ySubStart + ySubSize) <= mHeight);

        // Verify that the block follow one another nicely 
        Q_ASSERT(ySubStart == ySubStartNext);
        ySubStartNext = ySubStart + ySubSize;

        // Load data from disk
        if (yStep < yStepCount) {
            disparityBand->RasterIO(GF_Read, 0, yStep, xStepCount, 1, &disparityBuffer[1][2], xStepCount, 1, GDT_CFloat32, 0, 0);

            const int last = (1 + xStepCount) * 2;
            if (constantEdgeExtend) {
                disparityBuffer[1][0] = disparityBuffer[1][2];
                disparityBuffer[1][1] = disparityBuffer[1][3];                
                disparityBuffer[1][last+0] = disparityBuffer[1][last-2];
                disparityBuffer[1][last+1] = disparityBuffer[1][last-1];
            } else { // zero edge-extend
                disparityBuffer[1][0] = 0.0f;
                disparityBuffer[1][1] = 0.0f;
                disparityBuffer[1][last+0] = 0.0f;
                disparityBuffer[1][last+1] = 0.0f;
            }        
        } else if (constantEdgeExtend) {            
            disparityBuffer[1] = disparityBuffer[0];
        } else { // zero edge-extend
            memset(disparityBuffer[1], 0, (1 + xStepCount + 1) * 2 * sizeof(float *));
        }
        
        int xSubStartNext = 0;
        
        for (int xStep = 0; xStep <= xStepCount; ++xStep)
        {
            if (monitor.IsCanceled()) {
                monitor.SetProgress(workBase + workUnit * (yStep + xStep / (double)(xStepCount + 1)));
                break;
            }

            //profileTimer.start();

            int xA = round((xStep - 0.5) * xStepSize); // left
            int xX = round((xStep + 0.0) * xStepSize); // middle
            int xB = round((xStep + 0.5) * xStepSize); // right

            int xSubSize = (0 < xStep && xStep < xStepCount) ? (xB - xA) : (xStep == 0) ? xB - xX : xX - xA;
            int xSubStart = (xStep == 0) ? xX : xA;

            Q_ASSERT(0 < xSubSize && xSubSize <= xBlockSize);
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
            
            //fprintf(f, "[%3d ms] ", profileTimer.elapsed());   

            //fprintf(f, "Step: (%2d,%2d), (%4d,%4d), (%4d,%4d), (%4d,%4d) W (%4d,%4d)-(%3d,%3d)-(%4d,%4d) B 0x%08X 0x%08X P 0x%08X 0x%08X 0x%08X 0x%08X", 
            //    xStep,yStep, xX,yY, xA,yA, xB,yB, 
            //    xSubStart, ySubStart, xSubSize, ySubSize, xSubStart + xSubSize - 1, ySubStart + ySubSize - 1,
            //    (int)disparityBuffer[0], (int)disparityBuffer[1],  // check that they are swopped
            //    (int)p11, (int)p12, (int)p21, (int)p22); // verify duplicate pointers for first/last row/columns

            //fprintf(f, "disparityMap->RasterIO(GF_Write, %4d, %4d, %4d, %4d, disparityBuffer, %4d, %4d, GDT_Float32, 0, 0)\n", 
            //    xSubStart, ySubStart, xSubSize, ySubSize, xSubSize, ySubSize);

            //profileTimer.start();

            Q_ASSERT(0 <= xSubStart && xSubStart < mWidth);
            Q_ASSERT(0 <= ySubStart && ySubStart < mHeight);
            Q_ASSERT((xSubStart + xSubSize) <= mWidth);
            Q_ASSERT((ySubStart + ySubSize) <= mHeight);

            CPLErr err;
            err = disparityMapX->RasterIO(GF_Write, xSubStart, ySubStart, xSubSize, ySubSize, disparityBufferX, xSubSize, ySubSize, GDT_Float32, 0, 0);             
            err = disparityMapY->RasterIO(GF_Write, xSubStart, ySubStart, xSubSize, ySubSize, disparityBufferY, xSubSize, ySubSize, GDT_Float32, 0, 0);

            //fprintf(f, " EXIT %d\n", profileTimer.elapsed());  
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

        //fprintf(f, "*** Row completed in %d ms\n", profileRowTimer.elapsed());  
    }

    //if (f != stdout) fclose(f);

    free(disparityBuffer2); 
    free(disparityBuffer1); 

    free(disparityBufferY);
    free(disparityBufferX);
}

/* ************************************************************************* */

template <typename T>
static int cubicConvolution(T data[], float shift[])
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

static void applyDisparityMap(QgsProgressMonitor &monitor,
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

    double workBase = monitor.GetProgress();
    double workUnit = monitor.GetWorkUnit() / (mHeight + 1);

    for(int y = 0; y < mHeight; ++y)
    {
        monitor.SetProgress(workBase + workUnit * y);

        if (monitor.IsCanceled()) 
            break;

        disparityMapX->RasterIO(GF_Read, 0, y, mWidth, 1, disparityBufferReal, mWidth, 1, GDT_Float32, 0, 0);
        disparityMapY->RasterIO(GF_Read, 0, y, mWidth, 1, disparityBufferImag, mWidth, 1, GDT_Float32, 0, 0);

        for(int x = 0; x < mWidth; ++x)
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

    free(outputBuffer);
    free(disparityBufferImag);
    free(disparityBufferReal);    
}

/* ************************************************************************* */

static void copyBand(QgsProgressMonitor &monitor,
    GDALRasterBand *srcRasterBand, 
    GDALRasterBand *dstRasterBand, 
    int xOffset = 0, 
    int yOffset = 0)
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
    void *scanline_read = (void *)((int)scanline + dataSize * (xOffset > 0 ? xOffset : 0));
    void *scanline_write = (void *)((int)scanline + dataSize * (xOffset >= 0 ? 0 : -xOffset));
    
    for (int y = 0; y < height; ++y)
    {
        if (monitor.IsCanceled())
            break;

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

void performImageAlignment(QgsProgressMonitor &monitor,
    QString outputPath,
    QString disparityXPath, 
    QString disparityYPath,
    QString disparityPath,
    QList<GDALRasterBand *> &mBands, 
    int nRefBand = 0, 
    int nBlockSize = 201)
{
    if (outputPath.isEmpty()) {
        monitor.Log("ERROR: OutputPath string cannot be empty!");
        monitor.Cancel();
        return;
    }

    Q_ASSERT(!outputPath.isEmpty());

    if (disparityXPath.isEmpty()) {
        disparityXPath = /*QDir::tempPath() + QDir::separator() +*/"J:/tmp/" "tmp_disparity_map_x.tif";
    }
    if (disparityYPath.isEmpty()) {
        disparityYPath = /*QDir::tempPath() + QDir::separator() +*/"J:/tmp/" "tmp_disparity_map_y.tif";
    }
    if (disparityPath.isEmpty()) {
        disparityPath = /*QDir::tempPath() + QDir::separator() +*/"J:/tmp/" "tmp_disparity_map.tif";
    }

    Q_ASSERT(mBands.size() > 1);
    Q_ASSERT(0 <= nRefBand && nRefBand < mBands.size());

    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");

    GDALDataType outputType = mBands[nRefBand]->GetRasterDataType();

    switch (outputType) {
        case GDT_Byte:
        case GDT_UInt16:
        case GDT_UInt32:
        case GDT_Int16:
        case GDT_Int32:
            break;
        default:
            monitor.Log("ERROR: Unknown data type. Only integer types are currently supported.");
            monitor.Cancel();
            return;
    }

    const int mWidth = mBands[nRefBand]->GetXSize();
    const int mHeight = mBands[nRefBand]->GetYSize();

    Q_ASSERT(nBlockSize > 0);
    Q_ASSERT(nBlockSize < mWidth);
    Q_ASSERT(nBlockSize < mHeight);
    Q_ASSERT(nBlockSize % 2 == 1);
    
    const int xStepCount = ceil(mWidth / double(nBlockSize));
    const int yStepCount = ceil(mHeight / double(nBlockSize));

    // Remove the old file
    unlink(disparityPath.toAscii().data());

    // Create the disparity file using the calculated StepCount as width and height
    GDALDataset *disparityDataset = driver->Create(disparityPath.toAscii().data(), xStepCount, yStepCount, mBands.size(), GDT_CFloat32, 0);
    if (disparityDataset == NULL) {
        monitor.Log("ERROR: Unable to create the disparity dataset.");
        monitor.Cancel();
        return;
    }   
    Q_CHECK_PTR(disparityDataset);
    
    // ------------------------------------------------------------------------
    // Stage 1: calculate the disparity map for all the bands
    // ------------------------------------------------------------------------

    monitor.Log("Calculating the disparity map...");
       
    double workBase = 2.0; 
    double workUnit = (50 - workBase) / mBands.size();

    for (int i = 0; i < mBands.size(); ++i)
    {
        if (monitor.IsCanceled())
            break;        

        monitor.SetProgress(workBase + workUnit * i);

        GDALRasterBand *disparityBand = disparityDataset->GetRasterBand(1+i);        

        if (i == nRefBand) {
            disparityBand->Fill(0.0, 0.0);            
            continue;
        }

        StatisticsState stats[2];       

        monitor.SetWorkUnit(workUnit * 0.95);
        monitor.Log(QString("Scanning band #%1 to create disparity map ...").arg(1+i));

        // Calculate the disparity map
        scanDisparityMap(monitor, mBands[nRefBand], mBands[i], disparityBand, stats);

        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * (i + 0.95));
        monitor.SetWorkUnit(workUnit * 0.05);
        monitor.Log(QString("Eliminating bad points from disparity map ..."));

        // Remove faulty points that is outside the mean/stddev area
        eliminateDisparityMap(monitor, disparityBand, stats);
    }

    if (monitor.IsCanceled()) {
        GDALClose(disparityDataset);
        return;
    }

    monitor.SetProgress(50.0);

    // ------------------------------------------------------------------------
    // Stage 2: transform the input band into the output image
    // ------------------------------------------------------------------------

    unlink(outputPath.toAscii().data()); // Remove the old file

    GDALDataset *outputDataSet = driver->Create(outputPath.toAscii().data(), mWidth, mHeight, mBands.size(), outputType, 0);
    Q_CHECK_PTR(outputDataSet);

    unlink(disparityXPath.toAscii().data());

    GDALDataset *disparityXDataSet = driver->Create(disparityXPath.toAscii().data(), mWidth, mHeight, mBands.size()-1, GDT_Float32, 0);
    Q_CHECK_PTR(disparityXDataSet);

    unlink(disparityYPath.toAscii().data());

    GDALDataset *disparityYDataSet = driver->Create(disparityYPath.toAscii().data(), mWidth, mHeight, mBands.size()-1, GDT_Float32, 0);
    Q_CHECK_PTR(disparityYDataSet);

    workBase = 50.0;
    workUnit = (98.0 - workBase) / mBands.size();

    for (int i = 0, j = 1; i < mBands.size(); ++i)
    {
        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * i);

        if (i == nRefBand) {
            /* Copy the reference band over to the output file */
            copyBand(monitor, mBands[i], outputDataSet->GetRasterBand(1+i));
            continue;
        }

        monitor.SetWorkUnit(workUnit * 0.65);
        monitor.Log(QString("Estimating disparity X & Y map for band #%1 ...").arg(1+i));

        estimateDisparityMap(monitor,
            disparityDataset->GetRasterBand(1+i),    
            disparityXDataSet->GetRasterBand(j), 
            disparityYDataSet->GetRasterBand(j));

        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * (i + 0.65));
        monitor.SetWorkUnit(workUnit * 0.45);

        monitor.Log(QString("Applying disparity map to band #%1 ...").arg(1+i));
        applyDisparityMap(monitor,
            mBands[i],
            disparityXDataSet->GetRasterBand(j), 
            disparityYDataSet->GetRasterBand(j), 
            outputDataSet->GetRasterBand(1+i) );

        j += 1;
    }

    GDALClose(disparityYDataSet);
    GDALClose(disparityXDataSet);
    GDALClose(outputDataSet);
    
    GDALClose(disparityDataset);

    if (monitor.IsCanceled()) {
        return;
    }

    monitor.SetProgress(98.0);

}

/* ************************************************************************* */

QgsImageAligner::QgsImageAligner()
{
  mRealDisparity = NULL;
  mInputDataset = NULL;
  mTranslateDataset = NULL;
  mDisparityReal = NULL;
  mDisparityImag = NULL;
}

QgsImageAligner::QgsImageAligner(QString inputPath, QString translatePath, int referenceBand, int unreferencedBand, int blockSize)
{
  mInputPath = inputPath;
  mTranslatePath = translatePath;
  mReferenceBand = referenceBand;
  mUnreferencedBand = unreferencedBand;
  if(blockSize % 2 != 0)
  {
      mBlockSize = blockSize;
  }
  else
  {
      //mBlockSize = blockSize + 1;
      mBlockSize = blockSize - 1;
  }
  mRealDisparity = NULL;
  mInputDataset = NULL;
  mTranslateDataset = NULL;
  mDisparityReal = NULL;
  mDisparityImag = NULL;
  mWidth = 0;
  mHeight = 0;
  mMax = 0;
  mBits = 255;
  open();
}

QgsImageAligner::~QgsImageAligner()
{
  if(mRealDisparity != NULL)
  {
    for(int i = 0; i < mHeight; i++)
    {
      delete [] mRealDisparity[i];
    }
    delete [] mRealDisparity;
    mRealDisparity = NULL;
  }
  if(mDisparityReal != NULL)
  {
    free(mDisparityReal);
    mDisparityReal = NULL;
  }
  if(mDisparityImag != NULL)
  {
    free(mDisparityImag);
    mDisparityImag = NULL;
  }
  if(mInputDataset != NULL)
  {
    GDALClose(mInputDataset);
    mInputDataset = NULL;
  }
  if(mTranslateDataset != NULL)
  {
    GDALClose(mTranslateDataset);
    mTranslateDataset = NULL;
  }
}

void QgsImageAligner::open()
{
  if (GDALGetDriverCount() == 0)
  {
    GDALAllRegister();
  }
  mInputDataset = (GDALDataset*) GDALOpen(mInputPath.toAscii().data(), GA_ReadOnly);
  mTranslateDataset = (GDALDataset*) GDALOpen(mTranslatePath.toAscii().data(), GA_ReadOnly);
  mWidth = mInputDataset->GetRasterXSize();
  mHeight = mInputDataset->GetRasterYSize();
  mBits = pow(2, (double)GDALGetDataTypeSize(mInputDataset->GetRasterBand(1)->GetRasterDataType()));
}

#if 0
void QgsImageAligner::eliminate(double sigmaWeight)
{
  int xSteps = ceil(mWidth / double(mBlockSize));
  int ySteps = ceil(mHeight / double(mBlockSize));
  double *x = new double[xSteps*ySteps];
  double *y = new double[xSteps*ySteps];
  
  QgsComplex *newFixed = new QgsComplex[xSteps*ySteps];
  for(int i = 0; i < xSteps*ySteps; i++)
  {
    newFixed[i] = QgsComplex(0.0, 0.0);
  }
    
  int xCounter = 0;
  int yCounter = 0;
  for(int j = 0; j < xSteps; j++)
  {
    for(int i = 0; i < ySteps; i++)
    {
	//if(fixed[xCounter].real() != 0.0) mRealDisparity[i][j].setReal(fixed[xCounter].real());
	x[xCounter] = mReal[i][j].real();
	xCounter++;
	//if(fixed[yCounter].imag() != 0.0) mRealDisparity[i][j].setImag(fixed[yCounter].imag());
	y[yCounter] = mReal[i][j].imag();
	yCounter++;
    }
  }
  
  double stdDeviationX = gsl_stats_sd(x, 1, xSteps);
  double stdDeviationY = gsl_stats_sd(y, 1, ySteps);
  double meanX = gsl_stats_mean(x, 1, xSteps);
  double meanY = gsl_stats_mean(y, 1, ySteps);
  int counter = 0;
  
  for(int j = 0; j < xSteps; j++)
  {
    for(int i = 0; i < ySteps; i++)
    {
	if(isBad(mReal[i][j].real(), stdDeviationX, meanX, sigmaWeight))
	{
	  double avg = 0.0;
	  int avgCounter = 0;
	  if(i-1 >= 0 && !isBad(mReal[i-1][j].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i-1][j].real(); avgCounter++;}
	  if(j-1 >= 0 && !isBad(mReal[i][j-1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i][j-1].real(); avgCounter++;}
	  if(i+1 < ySteps && !isBad(mReal[i+1][j].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i+1][j].real(); avgCounter++;}
	  if(j+1 < xSteps && !isBad(mReal[i][j+1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i][j+1].real(); avgCounter++;}
	  if(i+1 < ySteps && j+1 < xSteps && !isBad(mReal[i+1][j+1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i+1][j+1].real(); avgCounter++;}
	  if(i-1 >= 0 && j+1 < xSteps && !isBad(mReal[i-1][j+1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i-1][j+1].real(); avgCounter++;}
	  if(i+1 < ySteps && j-1 >= 0 && !isBad(mReal[i+1][j-1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i+1][j-1].real(); avgCounter++;}
	  if(i-1 >= 0 && j-1 >= 0 && !isBad(mReal[i-1][j-1].real(), stdDeviationX, meanX, sigmaWeight)){avg += mReal[i-1][j-1].real(); avgCounter++;}
	  mReal[i][j].setReal(avg / avgCounter);
	  newFixed[counter].setReal(mReal[i][j].real());
	}
	if(isBad(mReal[i][j].imag(), stdDeviationY, meanY, sigmaWeight))
	{
	  double avg = 0.0;
	  int avgCounter = 0;
	  if(i-1>=0 && !isBad(mReal[i-1][j].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i-1][j].imag(); avgCounter++;}
	  if(j-1>=0 && !isBad(mReal[i][j-1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i][j-1].imag(); avgCounter++;}
	  if(i+1< ySteps && !isBad(mReal[i+1][j].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i+1][j].imag(); avgCounter++;}
	  if(j+1< xSteps && !isBad(mReal[i][j+1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i][j+1].imag(); avgCounter++;}
	  if(i+1 < ySteps && j+1 < xSteps && !isBad(mReal[i+1][j+1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i+1][j+1].imag(); avgCounter++;}
	  if(i-1 >= 0 && j+1 < xSteps && !isBad(mReal[i-1][j+1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i-1][j+1].imag(); avgCounter++;}
	  if(i+1 < ySteps && j-1 >= 0 && !isBad(mReal[i+1][j-1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i+1][j-1].imag(); avgCounter++;}
	  if(i-1 >= 0 && j-1 >= 0 && !isBad(mReal[i-1][j-1].imag(), stdDeviationY, meanY, sigmaWeight)){avg += mReal[i-1][j-1].imag(); avgCounter++;}
	  mReal[i][j].setImag(avg / avgCounter);
	  newFixed[counter].setImag(mReal[i][j].imag());
	}
	counter++;
    }
  }
  
  int c1 = 0;
  int c2 = 0;
  for(int j = 0; j < mWidth; j++)
  {
      for(int i = 0; i < mHeight; i++)
      {
          if (mRealDisparity[i][j].real() != 0.0 && 
              mRealDisparity[i][j].real() == mRealDisparity[i][j].real() && 
              mRealDisparity[i][j].imag() != 0.0 && 
              mRealDisparity[i][j].imag() == mRealDisparity[i][j].imag())
          {
              mRealDisparity[i][j] = mReal[c1][c2];
              c1++;
              if(c1>=ySteps)
              {
                  c2++;
                  c1 = 0;
              }
          }
      }
  }
  delete [] x;
  delete [] y;
}

bool QgsImageAligner::isBad(double value, double deviation, double mean, double sigmaWeight)
{
  if(value > mean+(sigmaWeight*deviation) || value < mean-(sigmaWeight*deviation))
  {
    return true;
  }
  return false;
}

void QgsImageAligner::scan()
{
  double xSteps = ceil(mWidth / double(mBlockSize));
  double ySteps = ceil(mHeight / double(mBlockSize));
  double xDiff = (xSteps * mBlockSize - mWidth) / (xSteps - 1);
  double yDiff = (ySteps * mBlockSize - mHeight) / (ySteps - 1);
  
  // allocate memory and set it to zero
  mRealDisparity = new QgsComplex*[mHeight];
  for(int i = 0; i < mHeight; i++)
  {
    mRealDisparity[i] = new QgsComplex[mWidth];
    for(int j = 0; j < mWidth; j++)
    {
      mRealDisparity[i][j] = QgsComplex();
    }
  }
  
  // alocate memory and set it to zero
  mReal = new QgsComplex*[int(ySteps)];
  for(int i = 0; i < int(ySteps); i++)
  {
    mReal[i] = new QgsComplex[int(xSteps)];
    for(int j = 0; j < int(xSteps); j++)
    {
      mReal[i][j] = QgsComplex();
    }
  }
   
  int xCounter = 0;
  for(int x = 0; x < xSteps; x++)
  {
    int newX = floor((x + 0.5) * mBlockSize - x * xDiff);

    int yCounter = 0;
    for(int y = 0; y < ySteps; y++)
    {
      int newY = floor((y + 0.5) * mBlockSize - y * yDiff);

      double point[2];
      point[0] = newX;
      point[1] = newY;
      QgsComplex c;
      
      mRealDisparity[newY][newX] = findPoint(point, c);

      mReal[yCounter][xCounter] = mRealDisparity[newY][newX];

      /*
      if(mReal[yCounter][xCounter].real() == 0.0 && mReal[yCounter][xCounter].imag() == 0.0)
      {
	    mSnrTooLow.append(QgsComplex(yCounter, xCounter));
      }
      */

      yCounter++;
    }
    xCounter++;
  }

  // FIXME: why do we close the dataset here?
  GDALClose(mInputDataset);
  mInputDataset = (GDALDataset*) GDALOpen(mInputPath.toAscii().data(), GA_ReadOnly);
}

QgsComplex QgsImageAligner::findPoint(double *point, QgsComplex estimate)
{
  double newPoint[2];
  newPoint[0] = point[0] + estimate.real();
  newPoint[1] = point[1] + estimate.imag();

  QgsRegion region1, region2;
  uint max1 = region(mInputDataset->GetRasterBand(mReferenceBand), NULL, point, mBlockSize, region1);
  uint max2 = region(mTranslateDataset->GetRasterBand(mUnreferencedBand), NULL, newPoint, mBlockSize, region2);

  mMax = max1 > max2 ? max1 : max2;

  double sum2 = 0;
  for(int i = 0; i < (region2.width * region2.height); i++)
  {
    sum2 += region2.data[i];
  }
  
  //If chip is water
  /*
  if((mBits - sum2 / (region2.width*region2.height)) < 55)
  {
    cout<<"water ("<<point[0]<<", "<<point[1]<<")"<<endl;
    delete [] region1.data;
    delete [] region2.data;
    return QgsComplex();
  }
  */
  
  QgsRegionCorrelationResult corrData = QgsRegionCorrelator::findRegion(region1, region2, mBits);
  
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
    /*if(corrData.snr > -24 && corrData.snr < -27)
    {      return QgsComplex();
    }*/
  }
  else
  {
    //cout<<"This point could not be found in the other band"<<endl;
  }

  newPoint[0] = point[0] + corrX;
  newPoint[1] = point[1] + corrY;
  return QgsComplex(newPoint[0] - point[0], newPoint[1] - point[1]);
}

uint QgsImageAligner::region(GDALRasterBand* rasterBand, QMutex *mutex, double point[], int dimension, QgsRegion &region)
{
    Q_CHECK_PTR(rasterBand);

    int x = int(point[0] + 0.5);
    int y = int(point[1] + 0.5);

    region.width = dimension;
    region.height = dimension;
    region.data = new uint[region.width * region.height];

    //if (mutex) mutex->lock();
   
    rasterBand->RasterIO(GF_Read, x - region.width/2, y - region.height/2, region.width, region.height, region.data, region.width, region.height, GDT_UInt32, 0, 0);
    
    //if (mutex) mutex->unlock();

    uint max = 0;
    
    for(int i = 0; i < region.width * region.height; i++)
    {
        uint value = region.data[i];

        if (max < value)
            max = value;

        region.data[i] = mBits - value;
    }

    return max;
}

void QgsImageAligner::estimate()
{
  //Create empty disparity map
  mDisparityReal = (double*) malloc(mHeight*mWidth*sizeof(double));
  mDisparityImag = (double*) malloc(mHeight*mWidth*sizeof(double));
  /*mDisparityReal = new double*[mHeight];
  mDisparityImag = new double*[mHeight];
  
  for(int i = 0; i < mHeight; i++)
  {
    mDisparityReal[i] = new double[mWidth];
    mDisparityImag[i] = new double[mWidth];
  }*/

  int xSteps = ceil(mWidth / double(mBlockSize));
  int ySteps = ceil(mHeight / double(mBlockSize));
  double xDiff = (xSteps * mBlockSize - mWidth) / double(xSteps - 1);
  double yDiff = (ySteps * mBlockSize - mHeight) / double(ySteps - 1);
  int blockB = floor((mBlockSize + 1) / 2.0);
  
  int xTo = xSteps+1;
  int yTo = ySteps+1;
  for(int x = 0; x < xTo; x++)
  {
      int x1 = floor((x-0.5)*mBlockSize - fabs((double)(x-1))*xDiff);
      int x2 = floor((x+0.5)*mBlockSize - x*xDiff);

      if(!(x1 > 0 && x1 <= mWidth-blockB))
      {
          x1 = x2;
      }
      if(!(x2 > 0 && x2 <= mWidth-blockB))
      {
          x2 = x1;
      }

      int newX;
      int startX;
      if(x2 - x1)
      {
          newX = x2-x1;
          startX = x1;
      }
      else
      {
          newX = min(x1, mWidth-x1);
          startX = x1;
          if(x1 <= blockB)
          {
              startX = 0;
          }
      }

      for(int y = 0; y < yTo; y++)
      {
          int y1 = floor((y-0.5)*mBlockSize - fabs((double)(y-1))*yDiff);
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
              newY = y2-y1;
              startY = y1;
          }
          else
          {
              newY = min(y1, mHeight-y1);
              startY = y1;
              if(y1 <= blockB)
              {
                  startY = 0;
              }
          }

          QgsComplex p1;
          QgsComplex p2;
          QgsComplex p3;
          QgsComplex p4;

          if(y1 < mHeight && x1 < mWidth)
          {
              p1 = mRealDisparity[y1][x1];
          }
          else
          {
              p1 = QgsComplex(0.0, 0.0);
          }

          if(y1 < mHeight && x2 < mWidth)
          {
              p2 = mRealDisparity[y1][x2];
          }
          else
          {
              p2 = QgsComplex(0.0, 0.0);
          }

          if(y2 < mHeight && x1 < mWidth)
          {
              p3 = mRealDisparity[y2][x1];
          }
          else
          {
              p3 = QgsComplex(0.0, 0.0);
          }

          if(y2 < mHeight && x2 < mWidth)
          {
              p4 = mRealDisparity[y2][x2];
          }
          else
          {
              p4 = QgsComplex(0.0, 0.0);
          }

          for(int xSub = 0; xSub < newX; xSub++)
          {
              int newXSub = startX + xSub;
              QgsComplex top = (p2*double(xSub) + p1*double(newX-xSub)) / double(newX);
              QgsComplex bottom = (p4*double(xSub) + p3*double(newX-xSub)) / double(newX);
              for(int ySub = 0; ySub < newY; ySub++)
              {
                  int newYSub = startY + ySub;                  
                  QgsComplex middle = (top*double(newY - ySub) + bottom*double(ySub)) / double(newY);

                  //cout<<"Real2: "<<newY<<" "<<y1<<" "<<y2<<" "<<p1.imag()<<" "<<middle.imag()<<endl;
                  int index = newYSub*mWidth + newXSub;
                  
                  mDisparityReal[index] = middle.real();
                  mDisparityImag[index] = middle.imag();
              }
          }
      }
  }

  if (mRealDisparity != NULL)
  {
      for(int i = 0; i < mHeight; i++)
      {
          delete [] mRealDisparity[i];
      }
      delete [] mRealDisparity;
      mRealDisparity = NULL;
  }
}
#endif


#if 0
void QgsImageAligner::alignImageBand(GDALDataset *disparityMapX, GDALDataset *disparityMapY, GDALDataset *outputData, int nBand)
{
    Q_CHECK_PTR(disparityMapX);
    Q_CHECK_PTR(disparityMapY);
    Q_CHECK_PTR(outputData);

__asm int 3; // Hard code a debugger breakpoint in C++

    GDALRasterBand *disparityReal = disparityMapX->GetRasterBand(nBand);
    GDALRasterBand *disparityImag = disparityMapY->GetRasterBand(nBand);
    GDALRasterBand *dst = outputData->GetRasterBand(nBand);
    GDALRasterBand *src = mTranslateDataset->GetRasterBand(mUnreferencedBand);

    float* scanLineDisparityReal = ( float * ) CPLMalloc( sizeof( float ) * mWidth );
    float* scanLineDisparityImag = ( float * ) CPLMalloc( sizeof( float ) * mWidth );
    float* resultLine = ( float * ) CPLMalloc( sizeof( float ) * mWidth );

    float data[16];

    for(int y = 0; y < mHeight; y++)
    {
        disparityReal->RasterIO(GF_Read, 0, y, mWidth, 1, scanLineDisparityReal, mWidth, 1, GDT_Float32, 0, 0);
        disparityImag->RasterIO(GF_Read, 0, y, mWidth, 1, scanLineDisparityImag, mWidth, 1, GDT_Float32, 0, 0);

        for(int x = 0; x < mWidth; x++)
        {
            QgsComplex trans;
            trans.setReal(-scanLineDisparityReal[x]);
            trans.setImag(-scanLineDisparityImag[x]);   

            float newX1 = floor(x + trans.real());
            float newY1 = floor(y + trans.imag());
            float newX2 = x + trans.real() - newX1;
            float newY2 = y + trans.imag() - newY1;

            if (1 <= x && x < mWidth-2) 
            {
                src->RasterIO(GF_Read, newX1-1, newY1-1, 4, 4, data, 4, 4, GDT_Float32, 0, 0);

                float value = 1; //cubicConvolution(data, QgsComplex(newX2, newY2));


                resultLine[x] = value;
            }
            else
            {
                resultLine[x] = 0; // TODO: no data value
            }
        }

        dst->RasterIO(GF_Write, 0, y, mWidth, 1, resultLine, mWidth, 1, GDT_Float32, 0, 0);
    }
}


uint* QgsImageAligner::applyUInt()
{
  const int size = mWidth * mHeight; 
  // For SumbandilaSat, 8415 x 9588 = 80683020 pixels
  // => 4 bytes per int gives: 322732080 bytes = 315168 KiB = 307.78 MiB
  //  And we allocate memory of that size twice! 615.56 MiB
  // TODO: Need to refactor this code to reduce the memory consumption.
  uint *image = new uint[size];
  memset(image, 0, size * sizeof(image[0]));

#if 1  

  int *base = new int[size];
  mTranslateDataset->GetRasterBand(mUnreferencedBand)->RasterIO(GF_Read, 0, 0, mWidth, mHeight, base, mWidth, mHeight, GDT_Int32, 0, 0);
  for(int x = 0; x < mWidth; x++)
  {
    for(int y = 0; y < mHeight; y++)
    {
      QgsComplex trans(mDisparityReal[y*mWidth + x], mDisparityImag[y*mWidth + x]);
      trans = trans.negative();
      double newX = floor(x+trans.real());
      double newY = floor(y+trans.imag());
      double newX2 = x+trans.real()-newX;
      double newY2 = y+trans.imag()-newY;
      double data[16];
      
      QList<int> indexes;
      indexes.append((newY-1)*mWidth + (newX-1));
      indexes.append((newY-1)*mWidth + (newX+0));
      indexes.append((newY-1)*mWidth + (newX+1));
      indexes.append((newY-1)*mWidth + (newX+2));

      indexes.append((newY+0)*mWidth + (newX-1));
      indexes.append((newY+0)*mWidth + (newX+0));
      indexes.append((newY+0)*mWidth + (newX+1));
      indexes.append((newY+0)*mWidth + (newX+2));

      indexes.append((newY+1)*mWidth + (newX-1));
      indexes.append((newY+1)*mWidth + (newX+0));
      indexes.append((newY+1)*mWidth + (newX+1));
      indexes.append((newY+1)*mWidth + (newX+2));

      indexes.append((newY+2)*mWidth + (newX-1));
      indexes.append((newY+2)*mWidth + (newX+0));
      indexes.append((newY+2)*mWidth + (newX+1));
      indexes.append((newY+2)*mWidth + (newX+2));
      
      bool broke = false;
      for(int i = 0; i < indexes.size(); i++)
      {
          int idx = indexes[i];
          if (0 <= idx && idx < size) 
          {
              data[i] = base[idx];
          } 
          else 
          {
              broke = true;
              break;
          }
      }
      if(!broke)
      {
          int value = cubicConvolution(data, QgsComplex(newX2, newY2));
          if(!(uint(value) > mBits) && !(uint(value) > mMax))
          {
              image[((mHeight-y)*mWidth - (mWidth-x)) ] = uint(value);
          }
      }
    }
  }
  delete [] base;

#endif

  if(mDisparityReal != NULL)
  {
    free(mDisparityReal);
    mDisparityReal = NULL;
  }
  if(mDisparityImag != NULL)
  {
    free(mDisparityImag);
    mDisparityImag = NULL;
  }
  
  return image;
}

int* QgsImageAligner::applyInt()
{
  int size = mWidth*mHeight;
  int *image = new int[size];
  memset(image, 0, size * sizeof(image[0]));
  
#if 0
  int *base = new int[size];
  mTranslateDataset->GetRasterBand(mUnreferencedBand)->RasterIO(GF_Read, 0, 0, mWidth, mHeight, base, mWidth, mHeight, GDT_Int32, 0, 0);
  for(int x = 0; x < mWidth; x++)
  {
    for(int y = 0; y < mHeight; y++)
    {
      QgsComplex trans(mDisparityReal[y*mWidth + x], mDisparityImag[y*mWidth + x]);
      trans = trans.negative();
      double newX = floor(x+trans.real());
      double newY = floor(y+trans.imag());
      double newX2 = x+trans.real()-newX;
      double newY2 = y+trans.imag()-newY;
      double data[16];
      
      QList<int> indexes;
      indexes.append((newY-1)*mWidth + (newX-1));
      indexes.append((newY-1)*mWidth + (newX));
      indexes.append((newY-1)*mWidth + (newX+1));
      indexes.append((newY-1)*mWidth + (newX+2));
      indexes.append((newY)*mWidth + (newX-1));
      indexes.append((newY)*mWidth + (newX));
      indexes.append((newY)*mWidth + (newX+1));
      indexes.append((newY)*mWidth + (newX+2));
      indexes.append((newY+1)*mWidth + (newX-1));
      indexes.append((newY+1)*mWidth + (newX));
      indexes.append((newY+1)*mWidth + (newX+1));
      indexes.append((newY+1)*mWidth + (newX+2));
      indexes.append((newY+2)*mWidth + (newX-1));
      indexes.append((newY+2)*mWidth + (newX));
      indexes.append((newY+2)*mWidth + (newX+1));
      indexes.append((newY+2)*mWidth + (newX+2));
      
      bool broke = false;
      for(int i = 0; i < indexes.size(); i++)
      {
          int idx = indexes[i];
          if (0 <= idx && idx < size) 
          {
              data[i] = base[idx];
          }
          else 
          {
              broke = true;
              break;
          }
      }
      if(!broke)
      {
          int value = cubicConvolution(data, QgsComplex(newX2, newY2));
          if(!(uint(value) > mBits) && !(uint(value) > mMax))
          {
              image[((mHeight-y)*mWidth - (mWidth-x)) ] = int(value);
          }
      }
    }
  }
  delete [] base;
#endif  
  
  if(mDisparityReal != NULL)
  {
    free(mDisparityReal);
    mDisparityReal = NULL;
  }
  if(mDisparityImag != NULL)
  {
    free(mDisparityImag);
    mDisparityImag = NULL;
  }
  
  return image;
}

double* QgsImageAligner::applyFloat()
{
  int size = mWidth*mHeight;
  double *image = new double[size];
  memset(image, 0, size * sizeof(image[0]));
  
#if 0
  int *base = new int[size];
  mTranslateDataset->GetRasterBand(mUnreferencedBand)->RasterIO(GF_Read, 0, 0, mWidth, mHeight, base, mWidth, mHeight, GDT_Int32, 0, 0);
  for(int x = 0; x < mWidth; x++)
  {
    for(int y = 0; y < mHeight; y++)
    {
      QgsComplex trans(mDisparityReal[y*mWidth + x], mDisparityImag[y*mWidth + x]);
      trans = trans.negative();
      double newX = floor(x+trans.real());
      double newY = floor(y+trans.imag());
      double newX2 = x+trans.real()-newX;
      double newY2 = y+trans.imag()-newY;
      double data[16];
      
      QList<int> indexes;
      indexes.append((newY-1)*mWidth + (newX-1));
      indexes.append((newY-1)*mWidth + (newX));
      indexes.append((newY-1)*mWidth + (newX+1));
      indexes.append((newY-1)*mWidth + (newX+2));
      indexes.append((newY)*mWidth + (newX-1));
      indexes.append((newY)*mWidth + (newX));
      indexes.append((newY)*mWidth + (newX+1));
      indexes.append((newY)*mWidth + (newX+2));
      indexes.append((newY+1)*mWidth + (newX-1));
      indexes.append((newY+1)*mWidth + (newX));
      indexes.append((newY+1)*mWidth + (newX+1));
      indexes.append((newY+1)*mWidth + (newX+2));
      indexes.append((newY+2)*mWidth + (newX-1));
      indexes.append((newY+2)*mWidth + (newX));
      indexes.append((newY+2)*mWidth + (newX+1));
      indexes.append((newY+2)*mWidth + (newX+2));
      
      bool broke = false;
      for(int i = 0; i < indexes.size(); i++)
      {
          int idx = indexes[i];
          if (0 <= idx && idx < size) 
          {
              data[i] = base[idx];
          }
          else
          {
              broke = true;
              break;
          }
      }
      if(!broke)
      {
          int value = cubicConvolution(data, QgsComplex(newX2, newY2));
          if(!(uint(value) > mBits) && !(uint(value) > mMax))
          {
              image[((mHeight-y)*mWidth - (mWidth-x)) ] = double(value);
          }
      }
    }
  }
  delete [] base;
#endif  
  
  if(mDisparityReal != NULL)
  {
    free(mDisparityReal);
    mDisparityReal = NULL;
  }
  if(mDisparityImag != NULL)
  {
    free(mDisparityImag);
    mDisparityImag = NULL;
  }
  
  return image;
}
#endif

int QgsImageAligner::cubicConvolution(double *array, QgsComplex point)
{
  double xShift = point.real();
  double yShift = point.imag();
  
  double *xArray = new double[4];
  double *yArray = new double[4];
  xArray[0] = -(1/2.0)*pow(xShift, 3) + pow(xShift, 2) - (1/2.0)*xShift;
  xArray[1] = (3/2.0)*pow(xShift, 3) - (5/2.0)*pow(xShift, 2) + 1;
  xArray[2] = -(3/2.0)*pow(xShift, 3) + 2*pow(xShift, 2) + (1/2.0)*xShift;
  xArray[3] = (1/2.0)*pow(xShift, 3) - (1/2.0)*pow(xShift, 2);
  
  yArray[0] = -(1/2.0)*pow(yShift, 3) + pow(yShift, 2) - (1/2.0)*yShift;
  yArray[1] = (3/2.0)*pow(yShift, 3) - (5/2.0)*pow(yShift, 2) + 1;
  yArray[2] = -(3/2.0)*pow(yShift, 3) + 2*pow(yShift, 2) + (1/2.0)*yShift;
  yArray[3] = (1/2.0)*pow(yShift, 3) - (1/2.0)*pow(yShift, 2);
  
  int result = 0;
  for(int y = 0; y < 4; y++)
  {
    int rowContribution = 0;
    for(int x = 0; x < 4; x++)
    {
      rowContribution += xArray[x] * array[y*4 + x];
    }
    result += rowContribution * yArray[y];
  }
  
  delete [] xArray;
  delete [] yArray;
  
  return result;
}

#if 0
QgsPointDetectionThread::QgsPointDetectionThread(QMutex *mutex, GDALDataset *inputDataset, GDALDataset *translateDataset, int referenceBand, int unreferencedBand, int blockSize, int bits, int *max)
{
  mMutex = mutex;
  mInputDataset = inputDataset;
  mTranslateDataset = translateDataset;
  mReferenceBand = referenceBand;
  mUnreferencedBand = unreferencedBand;
  mBlockSize = blockSize;
  mBits = bits;
  mMax = max;
  mPoint = new double[2];
}

void QgsPointDetectionThread::setPoint(double *point, QgsComplex estimate)
{
  mPoint[0] = point[0];
  mPoint[1] = point[1];
  mEstimate = estimate;
}

void QgsPointDetectionThread::run()
{
  double newPoint[2];
  newPoint[0] = mPoint[0] + mEstimate.real();
  newPoint[1] = mPoint[1] + mEstimate.imag();
  double corrX = mEstimate.real();
  double corrY = mEstimate.imag();

  QgsRegion region1, region2;
  uint max1 = region(mInputDataset->GetRasterBand(mReferenceBand), mMutex, mPoint, mBlockSize, region1);
  uint max2 = region(mTranslateDataset->GetRasterBand(mUnreferencedBand), mMutex, newPoint, mBlockSize, region2);
  
  if (mMax != NULL) 
     *mMax = max1 > max2 ? max1 : max2;
  
  QgsRegionCorrelationResult corrData = QgsRegionCorrelator::findRegion(region1, region2, mBits);
  
  delete [] region1.data;
  delete [] region2.data;

  if(corrData.wasSet)
  {
    corrX = corrData.x;
    corrY = corrData.y;
    //cout<<"Point found: "<<corrX<<", "<<corrY<<endl;
  }
  else
  {
   // cout<<"This point could not be found in the other band"<<endl;
  }
  newPoint[0] = mPoint[0]+corrX;
  newPoint[1] = mPoint[1]+corrY;
  mResult = QgsComplex(newPoint[0]-mPoint[0], newPoint[1]-mPoint[1]);
}

uint QgsPointDetectionThread::region(GDALRasterBand *rasterBand, QMutex *mutex, double point[], int dimension, QgsRegion &region)
{
    Q_CHECK_PTR(rasterBand);
    Q_CHECK_PTR(mutex);

    int x = int(point[0] + 0.5);
    int y = int(point[1] + 0.5);
  
    region.width = dimension;
    region.height = dimension;
    region.data = new uint[region.width * region.height];
 
    mutex->lock();
    rasterBand->RasterIO(GF_Read, x - region.width/2, y - region.height/2, region.width, region.height, region.data, region.width, region.height, GDT_UInt32, 0, 0);
    mutex->unlock();

    int max = 0;

    for(int i = 0; i < (region.width * region.height); i++)
    {
        int value = region.data[i];
    
        if (max < value)
            max = value;

        region.data[i] = mBits - value;
    }
  
    return max;
}
#endif