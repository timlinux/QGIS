#include "qgscomplex.h"
#include "qgsimagealigner.h"
#include "qgsregioncorrelator.h"

#include "gsl/gsl_statistics_double.h"
#include "gsl/gsl_blas.h"

#include <QDir>
#include <QTime>

#include <float.h>

/* ************************************************************************* */

#if defined(Q_WS_WIN)
static inline float round(float value) { return floor(value + 0.5f); }
static inline double round(double value) { return floor(value + 0.5); }
static inline long double round(long double value) { return floor(value + 0.5); }
#endif

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

    template <typename T>
    void UpdateState(T value) 
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

void scanDisparityMap(QgsProgressMonitor &monitor,
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
    
    // Fill scanline with initial disparity estimates
    for (int i = 0; i < xStepCount; ++i)
    {
        scanline[2*i+0] = 0.0;
        scanline[2*i+1] = 0.0;
    }

    QgsRegionCorrelator::QgsRegion region1, region2;
    region1.data = (unsigned int *)VSIMalloc3(1+ceil(xStepSize), 1+ceil(yStepSize), sizeof(unsigned int));
    region2.data = (unsigned int *)VSIMalloc3(1+ceil(xStepSize), 1+ceil(yStepSize), sizeof(unsigned int));
    
    /* Initialise the statistics state variables */
    stats[0].InitialiseState();
    stats[1].InitialiseState();

    double workBase = monitor.GetProgress();
    double workUnit = monitor.GetWorkUnit() / yStepCount;

    for (int yStep = 0; yStep < yStepCount; ++yStep)
    {
        if (monitor.IsCanceled()) 
            break;

        monitor.SetProgress(workBase + workUnit * yStep);
        
        //int y = round((yStep + 0.5) * yStepSize); // center of block

        int yA = round((yStep + 0.0) * yStepSize);
        int yB = round((yStep + 1.0) * yStepSize);
        region1.height = region2.height = yB - yA;

        for (int xStep = 0; xStep < xStepCount; ++xStep)
        {
            if (monitor.IsCanceled()) {
                monitor.SetProgress(workBase + workUnit * (yStep + xStep / (double)xStepCount));
                break;            
            }

            //int x = round((xStep + 0.5) * xStepSize); // center of block

            int xA = round((xStep + 0.0) * xStepSize);
            int xB = round((xStep + 1.0) * xStepSize);
            region1.width = region2.width = xB - xA;

            const int idx = 2 * xStep;
            /* Initial estimate of the disparity for more efficient correlation: */
            int estX = scanline[idx+0] < FLT_MAX ? scanline[idx+0] : 0; 
            int estY = scanline[idx+1] < FLT_MAX ? scanline[idx+1] : 0;            
            // use previous scanline's values (if valid) for a closer search ..
            estX = clamp(estX, -xStepSize/2, +xStepSize/2); 
            estY = clamp(estY, -yStepSize/2, +yStepSize/2); 
            // .. and we clamp it to half the step size to prevent runaway values

            // Take into account the height and width of the source image. 
            int x1 = xA - estX;
            int y1 = yA - estY;
            int x2 = clamp(x1, 0, srcWidth);
            int y2 = clamp(y1, 0, srcHeight);
            int region2_width = std::min(region2.width, srcWidth - x2);
            int region2_height = std::min(region2.height, srcHeight - y2);
            unsigned int *region2_data = region2.data;
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

            err = refRasterBand->RasterIO(GF_Read, xA, yA, region1.width, region1.height, region1.data, region1.width, region1.height, GDT_UInt32, 0, 0);
            if (err) {
                // TODO: better error handling 

                //fprintf(f, "RasterIO(%4d,%4d) error: %d - %s\n", xA,yA, err, CPLGetLastErrorMsg());
            }

            err = srcRasterBand->RasterIO(GF_Read, x2, y2, region2_width, region2_height, region2_data, region2.width, region2_height, GDT_UInt32, 0, 0);
            if (err) {
                // TODO: better error handling 

                //fprintf(f, "RasterIO(%4d,%4d)(%3d,%3d)(%3d,%3d) error: %d - %s\n", 
                //    x2,y2, 
                //    region2_width, region2_height, 
                //    region2.width, region2.height, 
                //    err, CPLGetLastErrorMsg());
            }

            //-----------------------------------------------------------------

            QgsRegionCorrelator::QgsResult corrData = 
                QgsRegionCorrelator::findRegion(region1, region2, mBits);

            //-----------------------------------------------------------------

            if (corrData.wasSet)
            {
                float corrX = estX + corrData.x; 
                float corrY = estY + corrData.y;

                //cout<<"Point found("<<point[0]<<" "<<point[1]<<"): "<<corrX<<", "<<corrY<<endl;
                //cout<<"      match("<<point[0]<<","<<point[1]<<"): "<<corrData.match<<endl;
                //cout<<"        SNR("<<point[0]<<","<<point[1]<<") = "<<corrData.snr<<endl;

                //cout << "Point found(" << point[0] << " " << point[1] <<"): "
                //      << corrX << ", " << corrY
                //      << " match = " << corrData.match
                //      << " SNR = " << corrData.snr 
                //      << endl;

                //if (f) fprintf(stdout, 
                //    "Point (%4d,%4d):%9.4f,%9.4f, match=%8.5f, SNR=%6.3f, (%3d,%3d)\n",
                //        x, y, corrX, corrY, corrData.match, corrData.snr, estX, estY);

                /* Save values in output buffer */
                scanline[idx+0] = corrX;
                scanline[idx+1] = corrY;

                /* Update statistics variables (only valid values) */
                stats[0].UpdateState(corrX);
                stats[1].UpdateState(corrY);
            }
            else
            {
                //cout<<"This point could not be found in the other band"<<endl;

                //fprintf(f, "Point (%4d,%4d) could not be found, run=%3d, est=(%5.3f,%5.3f)\n",
                //    x, y, runTimeMs, estX, estY);

                // Use a large value. It will be fixed in during the elimination step later on.
                scanline[idx+0] = FLT_MAX;
                scanline[idx+1] = FLT_MAX;

                // we don't update the stats for invalid values
            }
        }

        // Write the scanline buffer to the image
        err = disparityBand->RasterIO(GF_Write, 0, yStep, xStepCount, 1, scanline, xStepCount, 1, GDT_CFloat32, 0, 0);
        if (err) {
            // TODO: better error handling
            
            //fprintf(f, "RasterIO(WRITE)(%4d,%4d)(%3d,%3d) error: %d - %s\n", 
            //    0, yStep, 
            //    xStepCount, 1, 
            //    err, CPLGetLastErrorMsg());
        }
    }

    VSIFree(region2.data);
    VSIFree(region1.data);    

    VSIFree(scanline);

    /* Update the statistical mean and std deviation values */
    stats[0].FinaliseState();
    stats[1].FinaliseState();

    // FIXME: How do we handle complex values
    //disparityBand->SetStatistics(stats[0].Min, stats[0].Max, stats[0].Mean, stats[0].StdDeviation);
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

static void estimateDisparityMap(QgsProgressMonitor &monitor,
    GDALRasterBand *disparityBand, 
    GDALRasterBand *disparityMapX, 
    GDALRasterBand *disparityMapY)
{
    Q_CHECK_PTR(disparityBand);
    Q_CHECK_PTR(disparityMapX);
    Q_CHECK_PTR(disparityMapY);

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

    const int xBlockSize = ceil(xStepSize);
    const int yBlockSize = ceil(yStepSize);

    float *disparityBufferX = (float *)malloc(xBlockSize*yBlockSize * sizeof(float)); 
    float *disparityBufferY = (float *)malloc(xBlockSize*yBlockSize * sizeof(float)); 

    float *disparityBuffer1 = (float *)malloc((1 + xStepCount + 1) * 2 * sizeof(float));
    float *disparityBuffer2 = (float *)malloc((1 + xStepCount + 1) * 2 * sizeof(float));    

    float *disparityBuffer[2];    

    if (constantEdgeExtend) {
        disparityBuffer[0] = disparityBuffer1;
        disparityBuffer[1] = disparityBuffer1;
    } else { // zero edge extend
        memset(disparityBuffer2, 0, (1 + xStepCount + 1) * 2 * sizeof(float));
        disparityBuffer[0] = disparityBuffer2;
        disparityBuffer[1] = disparityBuffer1;
    }

    double workBase = monitor.GetProgress();
    double workUnit = monitor.GetWorkUnit() / (yStepCount + 1);

    int ySubStartNext = 0;

    for (int yStep = 0; yStep <= yStepCount; ++yStep)
    {
        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * yStep);

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
            memset(disparityBuffer[1], 0, (1 + xStepCount + 1) * 2 * sizeof(float));
        }
        
        int xSubStartNext = 0;
        
        for (int xStep = 0; xStep <= xStepCount; ++xStep)
        {
            if (monitor.IsCanceled()) {
                monitor.SetProgress(workBase + workUnit * (yStep + xStep / (double)(xStepCount + 1)));
                break;
            }

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

                    // Reference: http://en.wikipedia.org/wiki/Bilinear_interpolation
                    QgsComplex top = (p2*double(xSub) + p1*double(xSubSize - xSub)) / double(xSubSize);
                    QgsComplex bottom = (p4*double(xSub) + p3*double(xSubSize - xSub)) / double(xSubSize);
                    QgsComplex middle = (top*double(ySubSize - ySub) + bottom*double(ySub)) / double(ySubSize);
                    // FIXME: We might want to replace this with bicubic interplation

                    int idx = xSub + ySub * xSubSize;
                    
                    disparityBufferX[idx] = middle.real();
                    disparityBufferY[idx] = middle.imag();

#if 0 // TODO: this is what we need to do to work out and apply the disparity on one step!

                    int srcX1 = floor(x - disparityBufferX[idx]);
                    int srcY1 = floor(y - disparityBufferY[idx]);

                    if ((1 <= srcX1 && srcX1 < inWidth-2) && 
                        (1 <= srcY1 && srcY1 < inHeight-2))
                    {
                        inputRasterData->RasterIO(GF_Read, srcX1-1, srcY1-1, 4, 4, data, 4, 4, inDataType, 0, 0);

                        float shift[2] = { 
                            x - disparityBufferX[idx] - srcX1, 
                            y - disparityBufferY[idx] - srcY1,
                        };

                        double value = bicubic_interpolation(data, inDataType, shift);

                        store_in_buffer(outputBuffer, idx, outDataType, value);

                        // Keep statistical record of the valid pixels
                        stats.UpdateState( value );
                    }
                    else 
                    {
                        store_in_buffer(outputBuffer, idx, outDataType, noDataValue);
                    }
#endif
                }
            }
            
            //fprintf(f, "Step: (%2d,%2d), (%4d,%4d), (%4d,%4d), (%4d,%4d) W (%4d,%4d)-(%3d,%3d)-(%4d,%4d) B 0x%08X 0x%08X P 0x%08X 0x%08X 0x%08X 0x%08X", 
            //    xStep,yStep, xX,yY, xA,yA, xB,yB, 
            //    xSubStart, ySubStart, xSubSize, ySubSize, xSubStart + xSubSize - 1, ySubStart + ySubSize - 1,
            //    (int)disparityBuffer[0], (int)disparityBuffer[1],  // check that they are swopped
            //    (int)p11, (int)p12, (int)p21, (int)p22); // verify duplicate pointers for first/last row/columns

            //fprintf(f, "disparityMap->RasterIO(GF_Write, %4d, %4d, %4d, %4d, disparityBuffer, %4d, %4d, GDT_Float32, 0, 0)\n", 
            //    xSubStart, ySubStart, xSubSize, ySubSize, xSubSize, ySubSize);

            Q_ASSERT(0 <= xSubStart && xSubStart < mWidth);
            Q_ASSERT(0 <= ySubStart && ySubStart < mHeight);
            Q_ASSERT((xSubStart + xSubSize) <= mWidth);
            Q_ASSERT((ySubStart + ySubSize) <= mHeight);

            CPLErr err;
            err = disparityMapX->RasterIO(GF_Write, xSubStart, ySubStart, xSubSize, ySubSize, disparityBufferX, xSubSize, ySubSize, GDT_Float32, 0, 0);             
            err = disparityMapY->RasterIO(GF_Write, xSubStart, ySubStart, xSubSize, ySubSize, disparityBufferY, xSubSize, ySubSize, GDT_Float32, 0, 0);

            //err = outputRasterData->RasterIO(GF_Write, xSubStart, ySubStart, xSubSize, ySubSize, outputBuffer, xSubSize, ySubSize, outDataType, 0, 0);
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
    }

    free(disparityBuffer2); 
    free(disparityBuffer1); 

    free(disparityBufferY);
    free(disparityBufferX);
}

/* ************************************************************************* */

template <typename T>
static double cubicConvolution(T data[], float shift[])
{
    Q_CHECK_PTR(data);
    Q_CHECK_PTR(shift);

    // References: http://en.wikipedia.org/wiki/Bicubic_interpolation

    double xShift = shift[0];
    double xShift2 = xShift*xShift;
    double xShift3 = xShift*xShift2;

    double xData[4] = {
        -(1/2.0)* xShift3 + (2/2.0)*xShift2 - (1/2.0)*xShift,
         (3/2.0)* xShift3 - (5/2.0)*xShift2 + (2/2.0),
        -(3/2.0)* xShift3 + (4/2.0)*xShift2 + (1/2.0)*xShift,
         (1/2.0)* xShift3 - (1/2.0)*xShift2,
    };

    double yShift = shift[1];
    double yShift2 = yShift*yShift;
    double yShift3 = yShift*yShift2;

    double yData[4] = {
        0.5 * (  -yShift3 + 2*yShift2 - yShift),
        0.5 * ( 3*yShift3 - 5*yShift2 + 2 ),
        0.5 * (-3*yShift3 + 4*yShift2 + yShift ),
        0.5 * (   yShift3 -   yShift2),
    };
    
    double result = 0.0;
    for(int y = 0; y < 4; y++)
    {
        double rowContribution = 0.0;        
        for(int x = 0; x < 4; x++)
        {
            rowContribution += xData[x] * data[4*y + x];
        }
        result += rowContribution * yData[y];
    }    
    return result;
}

static double bicubic_interpolation(void *data, GDALDataType dataType, float shift[])
{
    switch (dataType)
    {
        case GDT_Byte:
            return cubicConvolution((unsigned char *)data, shift);
        case GDT_UInt16:
            return cubicConvolution((unsigned short *)data, shift);
        case GDT_UInt32:
            return cubicConvolution((unsigned int *)data, shift);
        case GDT_Int16:
            return cubicConvolution((signed short *)data, shift);
        case GDT_Int32:
            return cubicConvolution((signed int *)data, shift);
        case GDT_Float32:
            return cubicConvolution((float *)data, shift);
        case GDT_Float64:
            return cubicConvolution((double *)data, shift);
        default:
            Q_ASSERT(0 && "Should not get here. Programming error!");
            return 0.0;
    }
}

static inline void store_in_buffer(void *buffer, int index, GDALDataType dataType, double value)
{
    Q_CHECK_PTR(buffer);
    Q_ASSERT(index >= 0);

    switch (dataType)
    {
        case GDT_Byte:
            ((unsigned char *)buffer)[index] = value;
            break;
        case GDT_UInt16:
            ((unsigned short *)buffer)[index] = value;
            break;
        case GDT_UInt32:
            ((unsigned int *)buffer)[index] = value;
            break;
        case GDT_Int16:
            ((signed short *)buffer)[index] = value;
            break;
        case GDT_Int32:
            ((signed int *)buffer)[index] = value;
            break;
        case GDT_Float32:
            ((float *)buffer)[index] = value;
            break;
        case GDT_Float64:
            ((double *)buffer)[index] = value;
            break;
        default:
            Q_ASSERT(0 && "Should not get here. Programming error!");
            break;
    }
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

    GDALDataType outDataType = outputRasterData->GetRasterDataType();
    //int outDataTypeSize = GDALGetDataTypeSize(outDataType);

    float *disparityBufferReal = (float *)malloc(mWidth * sizeof(float));
    float *disparityBufferImag = (float *)malloc(mWidth * sizeof(float));
    double *outputBuffer = (double *) malloc(mWidth * sizeof(double)); /* 'double' is the largest datatype */

    //double noDataValue = inputRasterData->GetNoDataValue();
    double noDataValue = 0.0; //outputRasterData->GetNoDataValue();
    outputRasterData->SetNoDataValue(noDataValue);

    int inWidth = inputRasterData->GetXSize();
    int inHeight = inputRasterData->GetYSize();

    GDALDataType inDataType = inputRasterData->GetRasterDataType();
    //int inDataTypeSize = GDALGetDataTypeSize(inDataType);
    double data[4*4]; /* 'double' is the largest datatype */

    StatisticsState stats;
    stats.InitialiseState();

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
                inputRasterData->RasterIO(GF_Read, srcX1-1, srcY1-1, 4, 4, data, 4, 4, inDataType, 0, 0);

                float shift[2] = { 
                    x - disparityBufferReal[x] - srcX1, 
                    y - disparityBufferImag[x] - srcY1,
                };

                double value = bicubic_interpolation(data, inDataType, shift);

                store_in_buffer(outputBuffer, x, outDataType, value);
                
                // Keep statistical record of the valid pixels
                stats.UpdateState( value );
            }
            else
            {
                store_in_buffer(outputBuffer, x, outDataType, noDataValue);
            }
        }

        outputRasterData->RasterIO(GF_Write, 0, y, mWidth, 1, outputBuffer, mWidth, 1, outDataType, 0, 0);
    }

    stats.FinaliseState();
    outputRasterData->SetStatistics(stats.Min, stats.Max, stats.Mean, stats.StdDeviation);

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
    void *scanline_read = (void *)((intptr_t)scanline + dataSize * (xOffset > 0 ? xOffset : 0));
    void *scanline_write = (void *)((intptr_t)scanline + dataSize * (xOffset >= 0 ? 0 : -xOffset));
    
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

void QgsImageAligner::performImageAlignment(QgsProgressMonitor &monitor,
    QString outputPath,
    QString disparityXPath, 
    QString disparityYPath,
    QString disparityGridPath,
    QList<GDALRasterBand *> &mBands, 
    int nRefBand = 0, 
    int nBlockSize = 201)
{
    if (outputPath.isEmpty()) {
        monitor.Log("ERROR: OutputPath string cannot be empty!");
        monitor.Cancel();
        return;
    }
    if (nBlockSize % 2 == 0) { 
        monitor.Log("WARNING: BlockSize must be an odd value. Increasing block size by one.");
        nBlockSize += 1;
    }

    Q_ASSERT(!outputPath.isEmpty());

    bool removeDisparityXFile = disparityXPath.isEmpty();
    bool removeDisparityYFile = disparityXPath.isEmpty();
    bool removeDisparityGridFile = disparityGridPath.isEmpty();

    QFileInfo o(outputPath);
    QString tmpPath = o.path() + QDir::separator();
    //QString tmpPath = QDir::tempPath() + QDir::separator();
    if (disparityXPath.isEmpty()) {
        disparityXPath = tmpPath + o.completeBaseName() + ".xmap" + ".tif";
    }
    if (disparityYPath.isEmpty()) {
        disparityYPath = tmpPath + o.completeBaseName() + ".ymap" + ".tif";
    }
    if (disparityGridPath.isEmpty()) {
        disparityGridPath = tmpPath + o.completeBaseName() + ".grid" + ".tif";
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

    monitor.SetProgress(1.0);

    // ------------------------------------------------------------------------
    // Stage 1: calculate the disparity map for all the bands
    // ------------------------------------------------------------------------

    // Remove the old file
    unlink(disparityGridPath.toAscii().data());

    // Create the disparity file using the calculated StepCount as width and height
    GDALDataset *disparityGridDataset = driver->Create(disparityGridPath.toAscii().data(), xStepCount, yStepCount, mBands.size(), GDT_CFloat32, 0);
    if (disparityGridDataset == NULL) {
        monitor.Log("ERROR: Unable to create the disparity dataset.");
        monitor.Cancel();
        return;
    }
    Q_CHECK_PTR(disparityGridDataset);
           
    double workBase = monitor.GetProgress(); 
    double workUnit = (50 - workBase) / (1 + 9*(mBands.size()-1));

    for (int i = 0, k = 0; i < mBands.size(); ++i)
    {
        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * k++);

        GDALRasterBand *disparityGridBand = disparityGridDataset->GetRasterBand(1+i);        

        if (i == nRefBand) {
            disparityGridBand->Fill(0.0, 0.0);
            continue;
        }

        StatisticsState stats[2];

        monitor.SetWorkUnit(workUnit * 8); k += 7;
        monitor.Log(QString("Scanning band #%1 to create disparity map ...").arg(1+i));

        // Calculate the disparity map
        scanDisparityMap(monitor, mBands[nRefBand], mBands[i], disparityGridBand, stats);

        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * k++);
        monitor.SetWorkUnit(workUnit);
        monitor.Log(QString("Eliminating bad points from disparity map ..."));

        // Remove faulty points that is outside the mean/stddev area
        eliminateDisparityMap(monitor, disparityGridBand, stats);
    }

    if (monitor.IsCanceled()) {
        GDALClose(disparityGridDataset);
        return;
    }

    monitor.SetProgress(50.0);

    // ------------------------------------------------------------------------
    // Stage 2: copy the metadata from reference to output image
    // ------------------------------------------------------------------------

    unlink(outputPath.toAscii().data()); // Remove the old file
    char ** papszOptions = NULL;

    papszOptions = CSLSetNameValue( papszOptions, "PHOTOMETRIC", "RGB" );
    papszOptions = CSLSetNameValue( papszOptions, "ALPHA", "NO" );    

    GDALDataset *outputDataSet = driver->Create(outputPath.toAscii().data(), mWidth, mHeight, mBands.size(), outputType, papszOptions);
    if (outputDataSet == NULL) {
        monitor.Log("ERROR: Unable to create the output image dataset.");
        monitor.Cancel();
        CSLDestroy(papszOptions);
        GDALClose(disparityGridDataset);
        return;
    }
    
    CSLDestroy(papszOptions);
    Q_CHECK_PTR(outputDataSet);

    monitor.Log("Coping image metadata to output image ...");
    {
        GDALDataset *refDataset = mBands[nRefBand]->GetDataset();
        
        /* Copy general metadata to output dataset. */
        char** papszMetadata = CSLDuplicate(refDataset->GetMetadata());
        /* Remove TIFFTAG_MINSAMPLEVALUE and TIFFTAG_MAXSAMPLEVALUE * 
         * if the data range may change due to further processing.  */
        char** papszIter = papszMetadata;
        while (papszIter && *papszIter)
        {
            if (EQUALN(*papszIter, "TIFFTAG_MINSAMPLEVALUE=", 23) ||
                EQUALN(*papszIter, "TIFFTAG_MAXSAMPLEVALUE=", 23))
            {
                CPLFree(*papszIter);
                memmove(papszIter, papszIter+1, sizeof(char*) * (CSLCount(papszIter+1)+1));
            }
            else 
            {
                papszIter++;
            }
        }
        outputDataSet->SetMetadata(papszMetadata);
        CSLDestroy(papszMetadata);

        /* Copy description metadata to output dataset, if any. */
        const char *pszDesc = mBands[nRefBand]->GetDescription();
        if (pszDesc && *pszDesc) {
            outputDataSet->SetDescription(pszDesc);
        }

        /* Copy projection metadata to output dataset, if any. */
        const char *pszProjection = refDataset->GetProjectionRef();
        if (pszProjection && *pszProjection) {
            outputDataSet->SetProjection(pszProjection);
        }

        /* Copy GCP metadata to output dataset, if any. */
        int nGCPs = refDataset->GetGCPCount();
        if (nGCPs > 0) {
            outputDataSet->SetGCPs(nGCPs, refDataset->GetGCPs(), refDataset->GetGCPProjection());
        }
    }

    if (monitor.IsCanceled()) {
        GDALClose(outputDataSet);
        GDALClose(disparityGridDataset);
        return;
    }

    monitor.SetProgress(51.0);

    // ------------------------------------------------------------------------
    // Stage 3: transform the input bands into the output image
    // ------------------------------------------------------------------------

    unlink(disparityXPath.toAscii().data());

    GDALDataset *disparityXDataSet = driver->Create(disparityXPath.toAscii().data(), mWidth, mHeight, mBands.size()-1, GDT_Float32, 0);
    Q_CHECK_PTR(disparityXDataSet);

    unlink(disparityYPath.toAscii().data());

    GDALDataset *disparityYDataSet = driver->Create(disparityYPath.toAscii().data(), mWidth, mHeight, mBands.size()-1, GDT_Float32, 0);
    Q_CHECK_PTR(disparityYDataSet);

    workBase = monitor.GetProgress();
    workUnit = (98.0 - workBase) / (2*mBands.size() - 1); 
    // Each band uses 2 workunits, except reference band which use only 1 workunit.
    monitor.SetWorkUnit(workUnit);

    for (int i = 0, j = 0, k = 0; i < mBands.size(); ++i)
    {
        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * k++);

        GDALRasterBand *outputRaster = outputDataSet->GetRasterBand(1+i);
    
        monitor.Log(QString("Copying metadata from band #%1 ...").arg(1+i));

        /* Copy band description to output band, if any. */
        const char *desc = mBands[i]->GetDescription();
        if (desc && *desc != 0) {
            outputRaster->SetDescription(desc);
        }

        /* Copy band metadata to output band */
        char** papszMetadata = mBands[i]->GetMetadata();
        outputRaster->SetMetadata(papszMetadata);

        /* Copy the no-data value to output band, if any. */
        int bSuccess = 0;
        double dfNoData = mBands[i]->GetNoDataValue(&bSuccess);
        if (bSuccess) {
            outputRaster->SetNoDataValue(dfNoData);
        }

        if (i == nRefBand) {
            monitor.Log(QString("Copying image data for reference band #%1 ...").arg(1+i));
            /* Copy the reference band over to the output file */
            copyBand(monitor, mBands[i], outputRaster);
            /* Calculate and save the statistics for the reference band */
            StatisticsState stats;
            outputRaster->ComputeStatistics(false, &stats.Min, &stats.Max, &stats.Mean, &stats.StdDeviation, NULL, NULL);
            outputRaster->SetStatistics(stats.Min, stats.Max, stats.Mean, stats.StdDeviation);
            continue;
        }

        j += 1;

        monitor.Log(QString("Estimating disparity X & Y map for band #%1 ...").arg(1+i));

        estimateDisparityMap(monitor,
            disparityGridDataset->GetRasterBand(1+i),    
            disparityXDataSet->GetRasterBand(j), 
            disparityYDataSet->GetRasterBand(j));

        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * k++);

        monitor.Log(QString("Applying disparity map to band #%1 ...").arg(1+i));
        applyDisparityMap(monitor,
            mBands[i],
            disparityXDataSet->GetRasterBand(j), 
            disparityYDataSet->GetRasterBand(j), 
            outputRaster);
    }

    GDALClose(disparityYDataSet);
    GDALClose(disparityXDataSet);

    GDALClose(disparityGridDataset);

    GDALClose(outputDataSet);

    if (monitor.IsCanceled()) {
        return;
    }

    monitor.SetProgress(98.0);

    /* Remove disparity files after use */
    if (removeDisparityXFile)
        unlink(disparityXPath.toAscii().data());
    if (removeDisparityYFile)
        unlink(disparityYPath.toAscii().data());
    if (removeDisparityGridFile)
        unlink(disparityGridPath.toAscii().data());
}

/* ************************************************************************* */

/* end of file */
