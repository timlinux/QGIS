#include "qgscomplex.h"
#include "qgsimagealigner.h"
#include "qgsregioncorrelator.h"

#include "gsl/gsl_statistics_double.h"
#include "gsl/gsl_blas.h"

#include <QDir>
#include <QTime>

#include <float.h>
#include <math.h>

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
        Variance = (Count*SumSq - Sum*Sum) / (Count*(Count-1));    
        StdDeviation = sqrt(Variance);
        // References: 
        //   http://www.gcpowertools.com/Help/FormulaReference/FunctionSTDEV.html
        //   http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
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

static double clampf(double value, double vmin, double vmax)
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

static CPLErr load_from_raster(GDALRasterBand *raster, int offX, int offY, void *buffer, int bufSizeX, int bufSizeY, GDALDataType dataType)
{
    Q_CHECK_PTR(raster);
    Q_CHECK_PTR(buffer);
    Q_ASSERT(bufSizeX > 0);
    Q_ASSERT(bufSizeY > 0);

    const int width = raster->GetXSize();
    const int height = raster->GetYSize();
    
    if (0 <= offX && offX < (width - bufSizeX) &&
        0 <= offY && offY < (height - bufSizeY)) 
    {
        // inside the raster area, call RasterIO 
        return raster->RasterIO(GF_Read, offX, offY, bufSizeX, bufSizeY, buffer, bufSizeX, bufSizeY, dataType, 0, 0);
    }

    const int dataTypeSize = GDALGetDataTypeSize(dataType)/8;

    // Initialise the buffer with zeros
    memset(buffer, 0, bufSizeX * bufSizeY * dataTypeSize);

    if (offX <= -bufSizeX || width <= offX ||
        offY <= -bufSizeY || height <= offY)
    {
        // outside the raster area, return zero-filled buffer
        return CE_None; 
    }

    // Recalculate values for a partial block
    int x1 = offX;
    int y1 = offY;
    int x2 = clamp(x1, 0, width-1);
    int y2 = clamp(y1, 0, height-1);    
    int xoff = x2 - x1;
    int yoff = y2 - y1;

    Q_ASSERT(xoff >= 0);
    Q_ASSERT(yoff >= 0);

    int off_idx = xoff + yoff * bufSizeX;

    Q_ASSERT(off_idx < bufSizeX * bufSizeY);

    void *buffer_ptr = (void *)((intptr_t)buffer + off_idx * dataTypeSize);
    int buffer_width = std::min(bufSizeX, width - x2) - xoff;
    int buffer_height = std::min(bufSizeY, height - y2) - yoff;

    Q_ASSERT(0 <= buffer_width  && buffer_width  <= bufSizeX);
    Q_ASSERT(0 <= buffer_height && buffer_height <= bufSizeY);

    // Load the partial block from the raster
    return raster->RasterIO(GF_Read, x2, y2, buffer_width, buffer_height, 
        buffer_ptr, buffer_width, buffer_height, dataType, 0, bufSizeX * dataTypeSize);
}

/* ************************************************************************* */

void scanDisparityMap(QgsProgressMonitor &monitor,
    GDALRasterBand *refRasterBand,  GDALRasterBand *srcRasterBand,
    GDALDataset *disparityGridDataset, int bandIndex, StatisticsState stats[])
{
    Q_CHECK_PTR(refRasterBand);
    Q_CHECK_PTR(srcRasterBand);
    Q_CHECK_PTR(disparityGridDataset);
    Q_CHECK_PTR(stats);
    
    Q_ASSERT(bandIndex > 0);
    Q_ASSERT(disparityGridDataset->GetRasterCount() >= ((bandIndex-1)+4));

    GDALRasterBand *disparityGridX = disparityGridDataset->GetRasterBand(bandIndex + 0);
    GDALRasterBand *disparityGridY = disparityGridDataset->GetRasterBand(bandIndex + 1);
    GDALRasterBand *disparityGridE = disparityGridDataset->GetRasterBand(bandIndex + 2);
    GDALRasterBand *disparityGridP = disparityGridDataset->GetRasterBand(bandIndex + 3);

    Q_CHECK_PTR(disparityGridX);
    Q_CHECK_PTR(disparityGridY);
    Q_CHECK_PTR(disparityGridE);
    Q_CHECK_PTR(disparityGridP);

    int mWidth = refRasterBand->GetXSize();
    int mHeight = refRasterBand->GetYSize();

    // FIXME: 8bit & 16bit is ok, but not 32bit, float and double (and complex!)
    int mBits = 1 << GDALGetDataTypeSize(refRasterBand->GetRasterDataType());

    Q_ASSERT(mWidth > 0);
    Q_ASSERT(mHeight > 0);

    int xStepCount = disparityGridDataset->GetRasterXSize();
    int yStepCount = disparityGridDataset->GetRasterYSize();

    Q_ASSERT(xStepCount > 0);
    Q_ASSERT(yStepCount > 0);
    Q_ASSERT(xStepCount < mWidth);
    Q_ASSERT(yStepCount < mHeight);

    double xStepSize = mWidth / double(xStepCount);
    double yStepSize = mHeight / double(yStepCount);

    float *scanline = (float *)malloc(xStepCount * 4 *sizeof(float));
    
    /* Initialise the statistics state variables */
    stats[0].InitialiseState();
    stats[1].InitialiseState();

    QgsRegionCorrelator correlator(256, 256, 10);

    QgsRegionCorrelator::QgsRegion region1, region2;
    region1.width = region2.width = correlator.getRegionWidth();
    region1.height = region2.height = correlator.getRegionHeight();
    region1.data = correlator.getRegion1();
    region2.data = correlator.getRegion2();

    Q_ASSERT(region1.width  >= 1+ceil(xStepSize));
    Q_ASSERT(region1.height >= 1+ceil(yStepSize));
    Q_ASSERT(region2.width  >= 1+ceil(xStepSize));
    Q_ASSERT(region2.height >= 1+ceil(yStepSize));
   
    /* Pick an area in the middle of the image and calculate the disparity */
    load_from_raster(refRasterBand, (mWidth-region1.width)/2, (mHeight-region1.height)/2, region1.data, region1.width, region1.height, GDT_UInt32);
    load_from_raster(srcRasterBand, (mWidth-region2.width)/2, (mHeight-region2.height)/2, region2.data, region2.width, region2.height, GDT_UInt32);
    
    double estXi, estYi, error, diff_phase;
    correlator.findRegion(mBits, estXi, estYi, &error, &diff_phase);

    /* Fill scanline with initial disparity estimates */
    for (int i = 0; i < xStepCount; ++i)
    {
        scanline[4*i+0] = estXi;
        scanline[4*i+1] = estYi;
    }

    double workBase = monitor.GetProgress();
    double workUnit = monitor.GetWorkUnit() / yStepCount;

    for (int yStep = 0; yStep < yStepCount; ++yStep)
    {
        if (monitor.IsCanceled()) 
            break;

        monitor.SetProgress(workBase + workUnit * yStep);

        int y = round((yStep + 0.5) * yStepSize); // center of block
        int yA = y - region1.width/2;

        for (int xStep = 0; xStep < xStepCount; ++xStep)
        {
            if (monitor.IsCanceled()) {
                monitor.SetProgress(workBase + workUnit * (yStep + xStep / (double)xStepCount));
                break;            
            }

            int x = round((xStep + 0.5) * xStepSize); // center of block
            int xA = x - region1.width/2;

            const int idx = 4 * xStep;
            /* Initial estimate of the disparity for more efficient correlation: */
            int estX = scanline[idx+0] < FLT_MAX ? scanline[idx+0] : estXi;
            int estY = scanline[idx+1] < FLT_MAX ? scanline[idx+1] : estYi;
            // use previous scanline's values (if valid) for a closer search ..
            estX = clamp(estX, -xStepSize/2, +xStepSize/2); 
            estY = clamp(estY, -yStepSize/2, +yStepSize/2); 
            // .. and we clamp it to half the step size to prevent runaway values

            // Calculate the offset of the region in the source image. 
            int x1 = xA - estX;
            int y1 = yA - estY;

            // Load the data from the reference and source images
            load_from_raster(refRasterBand, xA, yA, region1.data, region1.width, region1.height, GDT_UInt32);
            load_from_raster(srcRasterBand, x1, y1, region2.data, region2.width, region2.height, GDT_UInt32);

            // Calculate the column/row shift between the two regions
            double colShift, rowShift;
            double nrmsError, diffPhase;
            
            correlator.findRegion(mBits, colShift, rowShift, &nrmsError, &diffPhase);

            colShift += estX;
            rowShift += estY;

            // Save values in output buffer
            scanline[idx+0] = colShift;
            scanline[idx+1] = rowShift;
            scanline[idx+2] = nrmsError;
            scanline[idx+3] = diffPhase;

            // Update statistics variables
            stats[0].UpdateState(colShift);
            stats[1].UpdateState(rowShift);
        }

        // Write the scanline buffer to the image ([0] column shift, [1] row shift, [2] normalised RMS error, [3] diff phase)
        disparityGridX->RasterIO(GF_Write, 0, yStep, xStepCount, 1, &scanline[0], xStepCount, 1, GDT_Float32, 4*sizeof(float), 0);
        disparityGridY->RasterIO(GF_Write, 0, yStep, xStepCount, 1, &scanline[1], xStepCount, 1, GDT_Float32, 4*sizeof(float), 0);
        disparityGridE->RasterIO(GF_Write, 0, yStep, xStepCount, 1, &scanline[2], xStepCount, 1, GDT_Float32, 4*sizeof(float), 0);
        disparityGridP->RasterIO(GF_Write, 0, yStep, xStepCount, 1, &scanline[3], xStepCount, 1, GDT_Float32, 4*sizeof(float), 0);
    }

    free(scanline);

    /* Update the statistical mean and std deviation values */
    stats[0].FinaliseState();
    stats[1].FinaliseState();
    
    disparityGridX->SetStatistics(stats[0].Min, stats[0].Max, stats[0].Mean, stats[0].StdDeviation);
    disparityGridY->SetStatistics(stats[1].Min, stats[1].Max, stats[1].Mean, stats[1].StdDeviation);
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
    GDALDataset *disparityGridDataset, int bandIndex, 
    StatisticsState stats[], 
    double sigmaWeight = 2.0)
{
    Q_CHECK_PTR(disparityGridDataset);
    Q_CHECK_PTR(stats);

    Q_ASSERT(bandIndex > 0);
    Q_ASSERT(disparityGridDataset->GetRasterCount() >= bandIndex+1);

    GDALRasterBand *disparityGridX = disparityGridDataset->GetRasterBand(bandIndex + 0);
    GDALRasterBand *disparityGridY = disparityGridDataset->GetRasterBand(bandIndex + 1);

    Q_CHECK_PTR(disparityGridX);
    Q_CHECK_PTR(disparityGridY);

    int xStepCount = disparityGridDataset->GetRasterXSize();
    int yStepCount = disparityGridDataset->GetRasterYSize();

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
    disparityGridX->RasterIO(GF_Read, 0, 0, xStepCount, 1, &scanline[1][0], xStepCount, 1, GDT_Float32, 2*sizeof(float), xStepCount*2*sizeof(float));
    disparityGridY->RasterIO(GF_Read, 0, 0, xStepCount, 1, &scanline[1][1], xStepCount, 1, GDT_Float32, 2*sizeof(float), xStepCount*2*sizeof(float));

    // Correct all points lying outside the mean/stdev range
    for (int yStep = 0; yStep < yStepCount; ++yStep) 
    {
        if ((yStepCount & 8) == 0) // 8 times rate reduction
            monitor.SetProgress(workBase + workUnit * yStep);

        if (monitor.IsCanceled())
            break;

        // Load the next scanline from disk
        if (yStep < yStepCount-1) {
            disparityGridX->RasterIO(GF_Read, 0, yStep+1, xStepCount, 1, &scanline[2][0], xStepCount, 1, GDT_Float32, 2*sizeof(float), 0);
            disparityGridY->RasterIO(GF_Read, 0, yStep+1, xStepCount, 1, &scanline[2][1], xStepCount, 1, GDT_Float32, 2*sizeof(float), 0);
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
        disparityGridX->RasterIO(GF_Write, 0, yStep, xStepCount, 1, &scanlineO[0], xStepCount, 1, GDT_Float32, 2*sizeof(float), 0);
        disparityGridY->RasterIO(GF_Write, 0, yStep, xStepCount, 1, &scanlineO[1], xStepCount, 1, GDT_Float32, 2*sizeof(float), 0);

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

template <typename T>
static void bicubic_interpolate_complex(T data[], int elementCount,  int lineStride, int start[], double frac[], double result[])
{
    Q_CHECK_PTR(data);
    Q_CHECK_PTR(start);
    Q_CHECK_PTR(frac);
    Q_CHECK_PTR(result);

    // References: http://en.wikipedia.org/wiki/bicubic_interpolation

    double xShift = frac[0];
    double xShift2 = xShift*xShift;
    double xShift3 = xShift*xShift2;

    double xData[4] = {
        -(1/2.0)* xShift3 + (2/2.0)*xShift2 - (1/2.0)*xShift,
         (3/2.0)* xShift3 - (5/2.0)*xShift2 + (2/2.0),
        -(3/2.0)* xShift3 + (4/2.0)*xShift2 + (1/2.0)*xShift,
         (1/2.0)* xShift3 - (1/2.0)*xShift2,
    };

    double yShift = frac[1];
    double yShift2 = yShift*yShift;
    double yShift3 = yShift*yShift2;

    double yData[4] = {
        0.5 * (  -yShift3 + 2*yShift2 - yShift),
        0.5 * ( 3*yShift3 - 5*yShift2 + 2 ),
        0.5 * (-3*yShift3 + 4*yShift2 + yShift ),
        0.5 * (   yShift3 -   yShift2),
    };

    double *rowContribution = (double *)alloca(elementCount * sizeof(double));        

    for (int i = 0; i < elementCount; i++) 
        result[i] = 0.0;

    for (int y = 0; y < 4; y++)
    {
        for (int i = 0; i < elementCount; i++)
            rowContribution[i] = 0.0;

        for (int x = 0; x < 4; x++)
        {
            const int idx = (x + start[0]) * elementCount + (y + start[1]) * lineStride;

            for (int i = 0; i < elementCount; i++)
            {
                rowContribution[i] += xData[x] * data[i + idx];
            }
        }

        for (int i = 0; i < elementCount; i++)
            result[i] += rowContribution[i] * yData[y];
    }
}

/* ************************************************************************* */

template <typename T>
static double cubicConvolution(T data[], double shift[])
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

/* ************************************************************************* */

static double bicubic_interpolation(void *data, GDALDataType dataType, double shift[])
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

/* ************************************************************************* */

static inline void store_in_buffer(void *buffer, int index, GDALDataType dataType, double value)
{
    Q_CHECK_PTR(buffer);
    Q_ASSERT(index >= 0);

    switch (dataType)
    {
        case GDT_Byte:
            ((unsigned char *)buffer)[index] = clampf(value, 0, 255.0);
            break;
        case GDT_UInt16:
            ((unsigned short *)buffer)[index] = clampf(value, 0, 65535.0);
            break;
        case GDT_UInt32:
            ((unsigned int *)buffer)[index] = clampf(value, 0, 4294967295.0);
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

/* ************************************************************************* */

static void transformDisparityMap(QgsProgressMonitor &monitor,
    GDALRasterBand *disparityGridX,
    GDALRasterBand *disparityGridY,
    GDALRasterBand *disparityMapX = NULL,
    GDALRasterBand *disparityMapY = NULL,
    GDALRasterBand *inputDataRaster = NULL,
    GDALRasterBand *outputDataRaster = NULL)
{
    Q_CHECK_PTR(disparityGridX);
    Q_CHECK_PTR(disparityGridY);

    int mWidth, mHeight;
    if (outputDataRaster)
    {
        mWidth = outputDataRaster->GetXSize();
        mHeight = outputDataRaster->GetYSize();
        
        if (disparityMapX) {
            Q_ASSERT(disparityMapX->GetXSize() == mWidth);
            Q_ASSERT(disparityMapX->GetXSize() == mHeight);
        }
        if (disparityMapY) {
            Q_ASSERT(disparityMapY->GetXSize() == mWidth);
            Q_ASSERT(disparityMapY->GetXSize() == mHeight);
        }
    } 
    else if (disparityMapX && disparityMapY)
    {
        mWidth = disparityMapX->GetXSize();
        mHeight = disparityMapX->GetYSize();

        Q_ASSERT(disparityMapX->GetXSize() == disparityMapY->GetXSize());
        Q_ASSERT(disparityMapX->GetYSize() == disparityMapY->GetYSize()); 
    }
    else
    {
        monitor.Log(QString("Neither the output data raster nor disparity map rasters is not defined!"));
        monitor.Log(QString("At least one of the two must be provided."));
        monitor.Cancel();
        return;
    }

    Q_ASSERT(mWidth > 0);
    Q_ASSERT(mHeight > 0);

    Q_ASSERT(disparityGridX->GetXSize() == disparityGridY->GetXSize());
    Q_ASSERT(disparityGridX->GetYSize() == disparityGridY->GetYSize()); 

    const int xGridStepCount = disparityGridX->GetXSize();
    const int yGridStepCount = disparityGridX->GetYSize();

    const double xGridStepSize = mWidth / double(xGridStepCount);
    const double yGridStepSize = mHeight / double(yGridStepCount);

    const bool constantEdgeExtend = true; // or false for zero edge-extend

    GDALDataType outDataType = outputDataRaster ? outputDataRaster->GetRasterDataType() : GDT_Byte;
    //const int outDataTypeSize = GDALGetDataTypeSize(outDataType) / 8;

    CPLErr err;
    StatisticsState stats;
    stats.InitialiseState();

    double workBase = monitor.GetProgress();
    double workUnit = monitor.GetWorkUnit() / (10 + mHeight);

    // Allocate working buffer memory for disparity map and output data
    float *disparityMapBufferX = (float *)calloc(mWidth, sizeof(float));
    float *disparityMapBufferY = (float *)calloc(mWidth, sizeof(float));
    void *outputBuffer = calloc(mWidth, sizeof(double)); /* 'double' is the largest datatype */

    const int disparityGridBufferSize = (2+xGridStepCount+2)*(2+yGridStepCount+2);
    Q_ASSERT(disparityGridBufferSize * 2*sizeof(float) < 128*1024*1024);
    float *disparityGridBuffer = (float *)calloc(disparityGridBufferSize, 2*sizeof(float)); 
    int disparityGridElementStride = 2;
    int disparityGridLineStride = (2 + xGridStepCount + 2) * disparityGridElementStride;
    float *disparityGrid = &disparityGridBuffer[2*disparityGridElementStride + 2*disparityGridLineStride];

    // Read the whole disparity grid into memory (it is an order of magnitude smaller that the disparity map)
    err = disparityGridX->RasterIO(GF_Read, 0, 0, xGridStepCount, yGridStepCount, &disparityGrid[0], xGridStepCount, yGridStepCount, GDT_Float32, disparityGridElementStride*sizeof(float), disparityGridLineStride*sizeof(float));
    err = disparityGridY->RasterIO(GF_Read, 0, 0, xGridStepCount, yGridStepCount, &disparityGrid[1], xGridStepCount, yGridStepCount, GDT_Float32, disparityGridElementStride*sizeof(float), disparityGridLineStride*sizeof(float));

    if (constantEdgeExtend) {
        // extend the edges with the value of the edge
        const int yLow = 0;
        const int yHigh = yGridStepCount-1;        
        float *rowM2 = &disparityGrid[(yLow -2) * disparityGridLineStride];
        float *rowM1 = &disparityGrid[(yLow -1) * disparityGridLineStride];
        float *rowM0 = &disparityGrid[(yLow -0) * disparityGridLineStride];
        float *rowP0 = &disparityGrid[(yHigh+0) * disparityGridLineStride];
        float *rowP1 = &disparityGrid[(yHigh+1) * disparityGridLineStride];
        float *rowP2 = &disparityGrid[(yHigh+2) * disparityGridLineStride];
        for (int xGridStep = 0; xGridStep < xGridStepCount; ++xGridStep) 
        {
            int elmIdx = xGridStep * disparityGridElementStride;
            for (int i = 0; i < disparityGridElementStride; ++i) {
                rowM2[i+elmIdx] = rowM1[i+elmIdx] = rowM0[i+elmIdx];
                rowP2[i+elmIdx] = rowP1[i+elmIdx] = rowP0[i+elmIdx];
             }
        }
        
        const int xLow = 0;
        const int xHigh = xGridStepCount-1;
        float *colM2 = &disparityGrid[(xLow -2) * disparityGridElementStride];
        float *colM1 = &disparityGrid[(xLow -1) * disparityGridElementStride];
        float *colM0 = &disparityGrid[(xLow -0) * disparityGridElementStride];
        float *colP0 = &disparityGrid[(xHigh+0) * disparityGridElementStride];
        float *colP1 = &disparityGrid[(xHigh+1) * disparityGridElementStride];
        float *colP2 = &disparityGrid[(xHigh+2) * disparityGridElementStride];
        for (int yGridStep = -2; yGridStep < yGridStepCount+2; ++yGridStep) 
        {
            int rowIdx = yGridStep * disparityGridLineStride;
            for (int i = 0; i < disparityGridElementStride; i++) {
                colM2[i + rowIdx] = colM1[i + rowIdx] = colM0[i + rowIdx];
                colP2[i + rowIdx] = colP1[i + rowIdx] = colP0[i + rowIdx];
            }
        }
    }

    // Initialise variables for input data memory structures
    const int inWidth = inputDataRaster ? inputDataRaster->GetXSize() : 1;
    const int inHeight = inputDataRaster ? inputDataRaster->GetYSize() : 1;
    const GDALDataType inDataType = inputDataRaster ? inputDataRaster->GetRasterDataType() : GDT_Byte;
    const int inDataTypeSize = GDALGetDataTypeSize(inDataType) / 8;
    const int inDataBufferSize = inWidth * inHeight * inDataTypeSize;
    const int inDataPixelStride = 1;
    const int inDataLineStride = inWidth * inDataPixelStride;
    
    errno = 0;
    // Allocate a buffer up to 256MiB of memory in size for input data. Anything over this use RasterIO function.
    void *inDataBuffer = (inputDataRaster && inDataBufferSize <= 256*1024*1024) ? malloc(inDataBufferSize) : NULL;
    intptr_t inDataBufferEnd = (intptr_t)inDataBuffer + inDataBufferSize;
    unsigned short *inData = (unsigned short *)inDataBuffer;

    if (inData != NULL) 
    {
        monitor.SetProgress(workBase + (3 * workUnit));

        // Read all the input data into memory for quicker accessing
        inputDataRaster->RasterIO(GF_Read, 0, 0, mWidth, mHeight, inData, mWidth, mHeight, inDataType, 0, inDataLineStride * inDataTypeSize);
    }
    else if (errno == ENOMEM)
    {
        monitor.Log(QString("Unable to allocate memory buffer. (%1 bytes) Falling back to file I/O algorithm.").arg(inDataBufferSize));
    }

    workBase += 10 * workUnit;
    monitor.SetProgress(workBase);

    for (int y = 0; y < mHeight; ++y) 
    {
        double yGridStep = (y - yGridStepSize/2) / yGridStepSize;

        if (monitor.IsCanceled())
            break;

        if (y % 8 == 0)
            monitor.SetProgress(workBase + workUnit * y);

        for (int x = 0; x < mWidth; ++x)
        {
            double xGridStep =  (x - xGridStepSize/2) / xGridStepSize;
            
            // ----------------------------------------------------------------

            double n;
            int start[2] = { floor(xGridStep), floor(yGridStep) };
            double frac[2] = { modf(xGridStep, &n), modf(yGridStep, &n) };
            double result[2];
#if 0
            {   // No interpolation
                int col1 = start[0]*disparityGridElementStride;
                int row1 = start[1]*disparityGridLineStride;
                float *p = &disparityGrid[col1 + row1];

                result[0] = p[0];
                result[1] = p[1];
            }
#elif 0
            {   // Bilinear interpolation
                int col1 = (start[0]+0)*disparityGridElementStride;
                int col2 = (start[0]+1)*disparityGridElementStride;
                int row1 = (start[1]+0)*disparityGridLineStride;
                int row2 = (start[1]+1)*disparityGridLineStride;

                float *p11 = &disparityGrid[col1 + row1]; // col = 1, row = 1
                float *p12 = &disparityGrid[col1 + row2]; // col = 1, row = 2
                float *p21 = &disparityGrid[col2 + row1]; // col = 2, row = 1
                float *p22 = &disparityGrid[col2 + row2]; // col = 2, row = 2

                QgsComplex p1(p11[0], p11[1]);
                QgsComplex p2(p21[0], p21[1]);
                QgsComplex p3(p12[0], p12[1]);
                QgsComplex p4(p22[0], p22[1]);

                if (xGridStep < 0.0) 
                    frac[0] = 1.0 + frac[0];
                if (yGridStep < 0.0)
                    frac[1] = 1.0 + frac[1];

                // Reference: http://en.wikipedia.org/wiki/Bilinear_interpolation
                QgsComplex top    = p1*(1.0 - frac[0]) + p2*frac[0];
                QgsComplex bottom = p3*(1.0 - frac[0]) + p4*frac[0];
                QgsComplex middle = top*(1.0 - frac[1]) + bottom*frac[1];
                
                result[0] = middle.real();
                result[1] = middle.imag();
            }
#else
            {   // Bicubic interpolation
                start[0] -= 1;
                start[1] -= 1;

                if (xGridStep < 0.0) 
                    frac[0] = 1.0 + frac[0];
                if (yGridStep < 0.0)
                    frac[1] = 1.0 + frac[1];

                bicubic_interpolate_complex(
                    disparityGrid, 
                    disparityGridElementStride, 
                    disparityGridLineStride, 
                    start,  
                    frac, 
                    result);
            }
#endif
            disparityMapBufferX[x] = result[0];
            disparityMapBufferY[x] = result[1];

            if (inputDataRaster == NULL)
                continue; // no input data, skip the reset of the loop

            // ----------------------------------------------------------------
            
            int source[2] = { floor(x - result[0]), floor(y - result[1]) };
            double shift[2] = { fabs(modf(x - result[0], &n)), fabs(modf(y - result[1], &n)) };
            double data[4*4]; /* 'double' is the largest datatype */

            if (inData) 
            {   // Fetch data from input data buffer
                int col = (source[0]-1) * inDataPixelStride;
                int row = (source[1]-1) * inDataLineStride;
                intptr_t top = (intptr_t)inData + (col+row)*inDataTypeSize;
                for (int j = 0; j < 4; j++) 
                {                 
                    row = j*inDataLineStride;
                    for (int i = 0; i < 4; i++) 
                    {
                        col = i*inDataPixelStride;                            
                        intptr_t p = top + (col+row)*inDataTypeSize;

                        bool valid = (intptr_t)inDataBuffer <= p && p < inDataBufferEnd;
                        
                        switch (inDataType) 
                        {
                        case GDT_Byte:
                            ((unsigned char *)data)[i+j*4] = valid ? *(unsigned char *)p : 0;
                            break;
                        case GDT_Int16:
                        case GDT_UInt16:
                            ((unsigned short *)data)[i+j*4] = valid ? *(unsigned short *)p : 0;
                            break;
                        case GDT_Int32:
                        case GDT_UInt32:
                            ((unsigned int *)data)[i+j*4] = valid ? *(unsigned int *)p : 0;
                            break;                            
                        case GDT_Float32:
                            ((float *)data)[i+j*4] = valid ? *(float *)p : 0;
                            break;                           
                        case GDT_Float64:
                            ((double *)data)[i+j*4] = valid ? *(double *)p : 0;
                            break;
                        default:
                            Q_ASSERT(0 && "Proramming error");
                            break;
                        }
                    }
                }
            }
            else 
            {   // Fetch directly from file (slower, used for LARGE files that don't fit in memory)                  
                if ((1 <= source[0] && source[0] < inWidth-2) &&
                    (1 <= source[1] && source[1] < inHeight-2))
                {
                    inputDataRaster->RasterIO(GF_Read, source[0]-1, source[1]-1, 4, 4, data, 4, 4, inDataType, 0, 0);
                }
                else
                {
                    load_from_raster(inputDataRaster, source[0]-1, source[1]-1, data, 4, 4, inDataType);
                }
            }

            double value = bicubic_interpolation(data, inDataType, shift);

            store_in_buffer(outputBuffer, x, outDataType, value);

            // Keep statistical record of the valid pixels
            stats.UpdateState( value );

            // ----------------------------------------------------------------
        }
        if (disparityMapX) {
            err = disparityMapX->RasterIO(GF_Write, 0, y, mWidth, 1, disparityMapBufferX, mWidth, 1, GDT_Float32, 0, 0);
        }
        if (disparityMapY) {
            err = disparityMapY->RasterIO(GF_Write, 0, y, mWidth, 1, disparityMapBufferY, mWidth, 1, GDT_Float32, 0, 0);
        }
        if (outputDataRaster) {
            err = outputDataRaster->RasterIO(GF_Write, 0, y, mWidth, 1, outputBuffer, mWidth, 1, outDataType, 0, 0);
        }
    }

    stats.FinaliseState();
    if (outputDataRaster) {
        outputDataRaster->SetStatistics(stats.Min, stats.Max, stats.Mean, stats.StdDeviation);
    }
   
    free(inDataBuffer);
    free(disparityGridBuffer);
    free(disparityMapBufferY);
    free(disparityMapBufferX);
    free(outputBuffer);
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

    bool removeDisparityGridFile = disparityGridPath.isEmpty();        
    if (disparityGridPath.isEmpty()) {
        QFileInfo o(outputPath);
        QString tmpPath = o.path() + QDir::separator();
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

    char ** papszOptions;

    // Remove the old file
    unlink(disparityGridPath.toAscii().data());

    papszOptions = NULL;
    papszOptions = CSLSetNameValue( papszOptions, "PHOTOMETRIC", "MINISBLACK" );
    papszOptions = CSLSetNameValue( papszOptions, "ALPHA", "NO" );    
   
    // Create the disparity file using the calculated StepCount as width and height
    GDALDataset *disparityGridDataset = driver->Create(disparityGridPath.toAscii().data(), xStepCount, yStepCount, (mBands.size()-1)*4, GDT_Float32, 0);
    if (disparityGridDataset == NULL) {
        monitor.Log("ERROR: Unable to create the disparity dataset.");
        monitor.Cancel();
        return;
    }
    
    Q_CHECK_PTR(disparityGridDataset);
    CSLDestroy(papszOptions);    
           
    double workBase = monitor.GetProgress(); 
    double workUnit = (50 - workBase) / (1 + 9*(mBands.size()-1));

    for (int i = 0, j = 0, k = 0; i < mBands.size(); ++i)
    {
        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * k++);

        if (i == nRefBand) {
            continue;
        }

        StatisticsState stats[2];

        monitor.SetWorkUnit(workUnit * 8); k += 7;
        monitor.Log(QString("Scanning band #%1 to create disparity map ...").arg(1+i));

        // Calculate the disparity map
        scanDisparityMap(monitor, mBands[nRefBand], mBands[i], disparityGridDataset, 1+j*4, stats);

        if (monitor.IsCanceled())
            break;

        monitor.SetProgress(workBase + workUnit * k++);
        monitor.SetWorkUnit(workUnit);
        //monitor.Log(QString("Eliminating bad points from disparity map ..."));

        // Remove faulty points that is outside the mean/stddev area
        //eliminateDisparityMap(monitor, disparityGridDataset, 1+j*4, stats);

        j += 1;
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

    papszOptions = NULL;
    if (mBands.size() > 2) {        
        papszOptions = CSLSetNameValue( papszOptions, "PHOTOMETRIC", "RGB" );
        papszOptions = CSLSetNameValue( papszOptions, "ALPHA", "NO" );    
    }

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

    disparityGridDataset->FlushCache();

    monitor.SetProgress(51.0);

    // ------------------------------------------------------------------------
    // Stage 3: transform the input bands into the output image
    // ------------------------------------------------------------------------

    GDALDataset *disparityXDataSet = NULL;
    GDALDataset *disparityYDataSet = NULL;
    
    if (!disparityXPath.isEmpty()) 
    {
        unlink(disparityXPath.toAscii().data());
        disparityXDataSet = driver->Create(disparityXPath.toAscii().data(), mWidth, mHeight, mBands.size()-1, GDT_Float32, 0);
    }    
    if (!disparityYPath.isEmpty())
    {
        unlink(disparityYPath.toAscii().data());
        disparityYDataSet = driver->Create(disparityYPath.toAscii().data(), mWidth, mHeight, mBands.size()-1, GDT_Float32, 0);
    }
    
    workBase = monitor.GetProgress();
    workUnit = (98.0 - workBase) / (2*mBands.size() + 1); 
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
        }
        else 
        {
            monitor.Log(QString("Transforming image data for image band #%1 ...").arg(1+i));
    
            transformDisparityMap(monitor,
                disparityGridDataset->GetRasterBand(1+4*j+0),
                disparityGridDataset->GetRasterBand(1+4*j+1),
                disparityXDataSet ? disparityXDataSet->GetRasterBand(1+j) : NULL,
                disparityYDataSet ? disparityYDataSet->GetRasterBand(1+j) : NULL,
                mBands[i],
                outputRaster);

            j += 1;
            k += 1;
        }
    }

    if (disparityYDataSet)
        GDALClose(disparityYDataSet);
    if (disparityXDataSet)
        GDALClose(disparityXDataSet);

    GDALClose(disparityGridDataset);

    GDALClose(outputDataSet);

    if (monitor.IsCanceled()) {
        return;
    }

    monitor.SetProgress(98.0);

    /* Remove disparity files after use */
    if (removeDisparityGridFile) {
        //unlink(disparityGridPath.toAscii().data());
    }
}

/* ************************************************************************* */

/* end of file */
