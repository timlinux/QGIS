#include "qgsregioncorrelator.h"

#ifndef _MSC_VER
#include <alloca.h>
#endif

#include <fftw3.h>
#include <gsl/gsl_fft_complex.h>
#include <gsl/gsl_complex_math.h>
#include <math.h>

#include <QList>

using namespace QgsRegionCorrelator;

/* Forward declarations */
//static double quantifySNR(gsl_complex *data, int dim, double x, double y);
//static double quantifyConfidence(gsl_complex *data, int dim);

/* ************************************************************************* */

/* Return a IEEE-compliant NAN value */
static inline double nan() 
{
    unsigned long long v = 0x7FFFFFFFFFFFFFFFll; 
    return *(double *)&v;
}

/* ************************************************************************* */

// Round up to next higher power of 2 (return x if it's already a power of 2).
// Reference: http://aggregate.org/MAGIC/#Next%20Largest%20Power%20of%202
static inline unsigned int PowerOfTwoRoundUp(unsigned int x) 
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return (x+1);
}

/* ************************************************************************* */

/**
 * Calculates the gradient between pixels
 */
template <typename T>
static gsl_complex *calculateGradient(gsl_complex *buffer, int rowStride,
    T *data, int width, int height, int bits, double order = 2.0)
{
    Q_CHECK_PTR(buffer);
    Q_ASSERT(rowStride > 0);
    Q_CHECK_PTR(data);

#define IDX(r,c) ((r)*width + (c))

    // Re-scaled all values to to 0 - 65535 (eg: -66 = 65535 - 66)
    int scale = bits; // FIXME: Why do we need to do this?

    double dx = order;
    double dy = order;

    int val;
    for (int r = 0; r < height; ++r)
    {
        val = data[IDX(r,1)] - data[IDX(r,0)];
        if(val < 0)
        {
            val = scale + val;
        }
        GSL_SET_REAL(&buffer[rowStride*r + 0], floor(val / dx));
    
        bool neg;
        for (int c = 1; c < width-1; ++c)
        {
            val = data[IDX(r, c+1)] - data[IDX(r, c-1)];
            neg = false;
            if(val < 0)
            {
                neg = true;
                val = scale - val;
            }
            GSL_SET_REAL(&buffer[rowStride*r + c], floor(val / (2*dx)) - (neg ? 1 : 0));
        }

        val = data[IDX(r, width-1)] - data[IDX(r, width-2)];
        neg = false;
        if(val < 0)
        {
            neg = true;
            val = scale + val;
        }
        GSL_SET_REAL(&buffer[rowStride*r + (width-1)], floor(val / dx) + (neg ? 1 : 0));
    }

    for (int c = 0; c < width; ++c) 
    {
        val = data[IDX(1,c)] - data[IDX(0,c)];
        if(val < 0)
        {
            val = scale + val;
        }
        GSL_SET_IMAG(&buffer[rowStride*0 + c], floor(val / dy));


        for (int r = 1; r < height-1; ++r) 
        {
            val = data[IDX(r+1,c)] - data[IDX(r-1,c)];
            if(val < 0)
            {
                val = scale + val;
            }
            GSL_SET_IMAG(&buffer[rowStride*r + c], floor(val / (2*dy)));

        }

        val = data[IDX(height-1,c)] - data[IDX(height-2,c)];
        if(val < 0)
        {
            val = scale - val;
        }
        GSL_SET_IMAG(&buffer[rowStride*(height-1) + c], floor(val / dy));
    }

    return buffer;

#undef IDX
}

/* ************************************************************************* */

/* Function that swap quadrants 1 and 3 as well as 2 and 4 */
static double *ifftShiftComplex(double *data, int dimension)
{
    // References: 
    //   http://www.mathworks.com/help/techdoc/ref/ifftshift.html
    //   http://www.mathworks.com/help/techdoc/ref/fftshift.html
    //   http://docs.scipy.org/doc/numpy/reference/generated/numpy.fft.ifftshift.html#numpy.fft.ifftshift
    //   http://docs.scipy.org/doc/numpy/reference/generated/numpy.fft.fftshift.html#numpy.fft.fftshift

    Q_ASSERT(dimension > 0);
    Q_ASSERT(dimension % 2 == 0); // must be even dimentions

    const int width = dimension;
    const int height = dimension;

    const int elemStride = 2; // complex numbers
    const int lineStride = elemStride * width;
    const int halfLineSize = 0.5 * lineStride * sizeof(double);

    double *tmpRow = (double *)alloca(2*halfLineSize);

    for (int r = 0; r < height/2; ++r)
    {
        int rr = r + height/2; // third and fourth quadrant's row
        int c0 = 0 + width/2;  // second and third quadrant's first column 

        // Calculate the start offset of each quadrant's row
        int idx_first  = lineStride * r + 0 * elemStride;
        int idx_second = lineStride * r + c0 * elemStride;
        int idx_third  = lineStride * rr + c0 * elemStride;
        int idx_fourth = lineStride * rr + 0 * elemStride;

        // Copy fourth and third quadrant's row to temporary memory
        memcpy(tmpRow, &data[idx_fourth], 2*halfLineSize);
        
        // Copy first quadrant's row to third quadrant
        memcpy(&data[idx_third], &data[idx_first], halfLineSize);

        // Copy second quadrant's row to fourth quadrant
        memcpy(&data[idx_fourth], &data[idx_second], halfLineSize);

        // Copy third quadrant's row to first quadrant
        memcpy(&data[idx_first], &tmpRow[2*c0], halfLineSize);

        // Copy fourth quadrant's row to second quadrant
        memcpy(&data[idx_second], tmpRow, halfLineSize);
    }

    return data;
}

/* ************************************************************************* */

/* 
 * This function calculates the offset between two regions by doing a 
 * correlation between the two arrays and finding the position with the
 * highest correlation value.
 */
QgsResult QgsRegionCorrelator::findRegion(QgsRegion &regionRef, QgsRegion &regionNew, int bits)
{  
    int maxW = PowerOfTwoRoundUp(std::max(regionRef.width, regionNew.width));
    int maxH = PowerOfTwoRoundUp(std::max(regionRef.height, regionNew.height));
    int max = std::max(maxW,maxH);

    gsl_complex *dataRef = (gsl_complex *)fftw_malloc(max * max * sizeof(gsl_complex));
    gsl_complex *dataNew = (gsl_complex *)fftw_malloc(max * max * sizeof(gsl_complex));

    for (int i = 0; i < regionRef.width * regionRef.height; ++i)
        regionRef.data[i] = bits - regionRef.data[i];
    for (int i = 0; i < regionNew.width * regionNew.height; ++i)
        regionNew.data[i] = bits - regionNew.data[i];    

    calculateGradient(dataRef, max, regionRef.data, regionRef.width, regionRef.height, bits);
    calculateGradient(dataNew, max, regionNew.data, regionNew.width, regionNew.height, bits);

    /* Calculate the Fourier Transform on 2D array. */

    fftw_plan plan;  
  
    plan = fftw_plan_dft_2d(max, max, (fftw_complex*)dataRef, (fftw_complex*)dataRef, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(plan);
    fftw_destroy_plan(plan);

    plan = fftw_plan_dft_2d(max, max, (fftw_complex*)dataNew, (fftw_complex*)dataNew, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(plan);
    fftw_destroy_plan(plan);

    gsl_complex *conj = (gsl_complex *)fftw_malloc(max * max * sizeof(gsl_complex));
    bool allzeros = true;

    for (int i = 0; i < max*max; ++i) 
    {
        // Conjugate the complex numbers. Equivalent to numpy: conj()
        GSL_SET_IMAG(&dataNew[i], -GSL_IMAG(dataNew[i]));
        //dataNew[i] = gsl_complex_conjugate(dataNew[i]);

        // Multiply and calculate the absolute value
        gsl_complex res = gsl_complex_mul(dataRef[i], dataNew[i]);
        double mag = gsl_complex_abs(res);
                
        if (mag > DBL_EPSILON) {
            GSL_SET_REAL(&conj[i], GSL_REAL(res) / mag);
            GSL_SET_IMAG(&conj[i], GSL_IMAG(res) / mag);
            //conj[i] = gsl_complex_div_real(conj[i], mag);
            allzeros = false;
        } else {
            GSL_SET_COMPLEX(&conj[i], 0, 0);
        }
    }

    fftw_free(dataNew);
    fftw_free(dataRef);

    //----------------------------------------------------
    // Optimization: quick exist if whole conj matrix is zeros
    if (allzeros) 
    {
        fftw_free(conj);
        return QgsRegionCorrelator::QgsResult(false);
    }
    //----------------------------------------------------

    /* Calculate the inverse fourier transform on 2D array. */

    // Inverse FFT operation
    plan = fftw_plan_dft_2d(max, max, (fftw_complex *)conj, (fftw_complex *)conj, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(plan);
    for (int i = 0; i < max*max; ++i) 
    {
      // Divide by max*max due to the scaling done by fftw
      GSL_SET_REAL(&conj[i], GSL_REAL(conj[i]) / max*max);
      GSL_SET_IMAG(&conj[i], GSL_IMAG(conj[i]) / max*max);
      //conj[i] = gsl_complex_div_real(conj[i], max*max);
    }
    fftw_destroy_plan(plan);

    gsl_complex *corr = (gsl_complex *)ifftShiftComplex((double*)conj, max);
    
    // Create an interest region in the middle of conj area

    int fromRow = (max - regionRef.width) / 2;
    int toRow   = (max + regionRef.width) / 2;
    int fromCol = (max - regionRef.height) / 2;
    int toCol   = (max + regionRef.height) / 2;

    gsl_complex *interest = &corr[max * fromRow + fromCol];
    
    // Find the index(es) with the maximum value

    double maxValue = -DBL_MAX;
    QList< std::pair<int,int> > indexes;
    indexes.append(std::make_pair(max/2,max/2));

    for (int r = 0; r < (toRow - fromRow); ++r)
    {
        for (int c = 0; c < (toCol - fromCol); ++c)
        {
            double value = GSL_REAL(interest[max * r + c]);

            if (maxValue < value) 
            {
                maxValue = value;
                indexes.clear();
            }             
            if (maxValue == value) 
            {
                indexes.append(std::make_pair(r,c));
            }
        }
    }

    Q_ASSERT(indexes.size() > 0);
    Q_ASSERT(maxValue != -DBL_MAX);
      
    double x1 = indexes[0].second; // column
    double y1 = indexes[0].first; // row
    double y = y1 - (regionRef.width / 2) - 1;
    double x = x1 - (regionRef.height / 2) - 1;
    double deltaX;
    double deltaY;

    int ySize = toRow - fromRow;
    int xSize = toCol - fromCol;

    try
    {
        //Delta Y
        try
        {
            int signDelta = -1;

            deltaY = 0.0;

            if (ySize > y1+1 && ySize > y1-1 && y1-1 >= 0)
            {
                double m = GSL_REAL(interest[max * int(y1-1) + int(x1)]);
                double n = GSL_REAL(interest[max * int(y1+1) + int(x1)]);
                
                if (m < n) 
                {
                    signDelta = +1;
                }

                double value = (signDelta < 0) ? m : n;

                if (ySize > (y1+signDelta) && value == maxValue)
                {
                    deltaY = 0.5 * signDelta;
                }
                else
                {
                    double a = value / (value - maxValue);
                    double b = value / (value + maxValue);

                    if (0 < a*signDelta && a*signDelta < 0.5) {
                        deltaY = a;
                    } else if (0 < b*signDelta && a*signDelta < 0.5) { // FIXME: should it not be b here? (second part)
                        deltaY = b;
                    } else  {
                        deltaY = std::min(fabs(a), fabs(b)) * signDelta;
                    }
                }
            }
        }
        catch(...)
        {
            deltaY = 0.0;
        }

        //Delta X
        try
        {
            int signDelta = -1;

            deltaX = 0.0;

            if (ySize > y1 && xSize > x1+1 && x1-1 >= 0)
            {
                double m = GSL_REAL(interest[max * int(y1) + int(x1-1)]);
                double n = GSL_REAL(interest[max * int(y1) + int(x1+1)]);

                if (m < n) 
                {
                    signDelta = +1;
                }

                double value = (signDelta < 0) ? m : n;

                if (xSize > x1+signDelta && value == maxValue) 
                {
                    deltaX = 0.5 * signDelta;
                }
                else
                {
                    double a = value / (value - maxValue);
                    double b = value / (value + maxValue);

                    if (0.0 < a*signDelta && a*signDelta < 0.5)
                    {
                        deltaX = a;
                    }
                    else if (0.0 < b*signDelta && a*signDelta < 0.5)
                    {
                        deltaX = b;
                    }
                    else
                    {
                        deltaX = std::min(fabs(a),fabs(b)) * signDelta;
                    }
                }
            }
        }
        catch(...)
        {
            deltaX = 0.0;
        }

        y += deltaY;
        x += deltaX;
    }
    catch(...)
    {
        x = 0.0;
        y = 0.0;
    }

#if 0 // FIXME: implement SNR and confidence functions
    double snr = quantifySNR(interest, xSize, x1, y1);
    double confidence = quantifyConfidence(interest, xSize, ySize);
#else
    double snr = 0;
    double confidence = 0;
#endif

    fftw_free(conj);

    return QgsRegionCorrelator::QgsResult(true, x, y, max, maxValue, confidence, snr);
}

/* ************************************************************************* */
#if 0 // DISABLED

static double quantifySNR(gsl_complex *data, int dim, double x, double y)
{    
    return nan(); // FIXME: not implemented yet

    Q_CHECK_PTR(data);
    Q_ASSERT(dim > 0);
    Q_ASSERT(x > 0);
    Q_ASSERT(y > 0);

    int r1 = y - 1;
    int r2 = y + 2;
    int c1 = x - 1;
    int c2 = x + 2;

    gsl_complex *subset = &data[dim * r1 + c1];

    double energySignal = 0.0;
    for (int r = r1; r < r2; ++r) {
        for (int c = c1; c < c2; ++c) {
            double value = GSL_REAL(subset[dim * r + c]);
            energySignal += value * value;
        }
    }      

    double energyNoise = 0.0;
    for (int i = 0; i < dim*dim; ++i) {
        double value = GSL_REAL(data[i]);
        energyNoise += value * value;
    }

    energyNoise -= energySignal;
    if (energyNoise > DBL_EPSILON)
    {
        if (energySignal > DBL_EPSILON)
        {
            return 10.0 * log10(energySignal / energyNoise);
        }
        else
        {
            return -50.0;
        }
    }
    else
    {
        return 50.0;
    }
}

static double quantifyConfidence(gsl_complex *data, int dim)
{
    return nan(); // FIXME: not implemented yet

    // Example: MATLAB code?
    //int points = array.size() * array[0].size();
    //arrayc = detect_local_maxima(array)*array
	//arrayc = np.reshape(arrayc,(1,Points))
	//arrayc.sort()
	//conf = arrayc[0][-1]/arrayc[0][-2]        
    //return 1-1/conf  
}

#endif
/* ************************************************************************* */
/* end of file */
