#include "qgsregioncorrelator.h"

#ifndef _MSC_VER
#include <alloca.h>
#endif

#include <math.h>
#include <gsl/gsl_fft_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_blas.h>

#include <QDebug>

/* ************************************************************************* */

#if defined(Q_WS_WIN)
static inline float round(float value) { return floor(value + 0.5f); }
static inline double round(double value) { return floor(value + 0.5); }
static inline long double round(long double value) { return floor(value + 0.5); }
#endif

/* ************************************************************************* */

static inline double fix(double value) 
{
    if (value < 0.0)
        return ceil(value);
    else
        return floor(value);
}  

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
    Q_ASSERT(width > 0);
    Q_ASSERT(height > 0);

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

static inline int idx_fftshift(int r, int c, int nr, int nc)
{
    int m = (r < nr/2) ? r + (nr+1)/2 : r - nr/2;
    int n = (c < nc/2) ? c + (nc+1)/2 : c - nc/2;
    return n + m * nc;
}

static inline int idx_ifftshift(int r, int c, int nr, int nc)
{
    int m = (r < (nr+1)/2) ? r + nr/2 : r - (nr+1)/2;
    int n = (c < (nc+1)/2) ? c + nc/2 : c - (nc+1)/2;
    return n + m * nc;
}

/* ************************************************************************* */

/* Function that swap quadrants 1 and 3 as well as 2 and 4 */
double *ifftShiftComplex(double *data, int nr, int nc)
{
    // References: 
    //   http://www.mathworks.com/help/techdoc/ref/ifftshift.html
    //   http://www.mathworks.com/help/techdoc/ref/fftshift.html
    //   http://docs.scipy.org/doc/numpy/reference/generated/numpy.fft.ifftshift.html#numpy.fft.ifftshift
    //   http://docs.scipy.org/doc/numpy/reference/generated/numpy.fft.fftshift.html#numpy.fft.fftshift

    Q_ASSERT(nr > 0);
    Q_ASSERT(nc > 0);

    Q_ASSERT(nr % 2 == 0); // must be even (odd not implemented yet)
    Q_ASSERT(nc % 2 == 0); // must be even (odd not implemented yet)

    const int width = nr;
    const int height = nc;

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

static void dftUpSample(gsl_complex in[], size_t nr, size_t nc, gsl_complex out[], size_t nor, size_t noc, int usfac = 1, double roff = 0.0, double coff = 0.0)
{
    Q_CHECK_PTR(in);  // input buffer
    Q_CHECK_PTR(out); // output buffer

    Q_ASSERT(nr > 0);
    Q_ASSERT(nc > 0);
    Q_ASSERT(nor > 0);
    Q_ASSERT(noc > 0);
    
    // MATLAB:
    //  % Compute kernels and obtain DFT by matrix products
    //  [nr,nc]=size(in)
    //  kernc = exp((-i*2*pi/(nc*usfac))*( ifftshift([0:nc-1]).' - floor(nc/2) )*( [0:noc-1] - coff ))
    //  kernr = exp((-i*2*pi/(nr*usfac))*( [0:nor-1].' - roff )*( ifftshift([0:nr-1]) - floor(nr/2)  ))
    //  out = kernr * in * kernc
    //
    gsl_complex *kernr = (gsl_complex *)malloc(nor*nr * sizeof(gsl_complex));
    gsl_complex *kernc = (gsl_complex *)malloc(nc*noc * sizeof(gsl_complex));
    gsl_complex *tmp   = (gsl_complex *)malloc(nor*nc * sizeof(gsl_complex));

    double ifftshift, factor;

    factor = -2*M_PI/(nr*usfac);    
    for (size_t r = 0; r < nor; ++r)
    {
        for (size_t c = 0; c < nr; ++c)
        {
            ifftshift = (c < (nr+1)/2) ? c + nr/2 : c - (nr+1)/2;
            ifftshift -= floor(nr/2.0);

            gsl_complex v;
            GSL_SET_COMPLEX(&v, 0, factor * (r - roff) * ifftshift);
            kernr[c + r*nr] = gsl_complex_exp(v);
        }
    }

    factor = -2*M_PI/(nc*usfac);        
    for (size_t r = 0; r < nc; ++r)
    {
        ifftshift = (r < (nc+1)/2) ? r + nc/2 : r - (nc+1)/2;
        ifftshift -= floor(nc/2.0);

        for (size_t c = 0; c < noc; ++c)
        {
            gsl_complex v;
            GSL_SET_COMPLEX(&v, 0, factor * ifftshift * (c - coff));
            kernc[c + r*noc] = gsl_complex_exp(v);
        }
    }

    gsl_complex zero, one;
    GSL_SET_COMPLEX(&zero, 0.0, 0.0);
    GSL_SET_COMPLEX(&one,  1.0, 0.0);

    gsl_matrix_complex_view A = gsl_matrix_complex_view_array((double *)kernr, nor, nr);
    gsl_matrix_complex_view B = gsl_matrix_complex_view_array((double *)in,     nr, nc);
    gsl_matrix_complex_view C = gsl_matrix_complex_view_array((double *)kernc,  nc, noc);
    gsl_matrix_complex_view D = gsl_matrix_complex_view_array((double *)tmp,   nor, nc);
    gsl_matrix_complex_view E = gsl_matrix_complex_view_array((double *)out,   nor, noc);

    /* Compute: out = kernr * in * kernc */ 
    gsl_blas_zgemm(CblasNoTrans, CblasNoTrans, one, &A.matrix, &B.matrix, zero, &D.matrix);
    gsl_blas_zgemm(CblasNoTrans, CblasNoTrans, one, &D.matrix, &C.matrix, zero, &E.matrix);

    free(tmp);
    free(kernc);
    free(kernr);
}

/* ************************************************************************* */

static int gsl_complex_find_max(gsl_complex *data, int nr, int nc, int &rloc, int &cloc, gsl_complex &CCmax)
{
    Q_CHECK_PTR(data);
    Q_ASSERT(nr > 0);
    Q_ASSERT(nc > 0);

    // Find the index with the maximum real value

    int index = 0;
    int counter = 1;
    CCmax = data[0];
    
    for (int i = 1; i < nr*nc; ++i)
    {
        gsl_complex *v = &data[i];
        
        if (GSL_REAL(CCmax) < GSL_COMPLEX_P_REAL(v)) 
        {
            CCmax = *v;
            index = i;
            counter = 1;
        }
        else if (GSL_REAL(CCmax) == GSL_COMPLEX_P_REAL(v)) 
        {
            counter++;
        }
    }

    rloc = index / nc;
    cloc = index % nc;
    return counter;
}

/* ************************************************************************* */

/* The constructor for the region corerelator class. Here we allocate all the
 * memory that will be needed by the findRegion method. We also do the FFTW
 * planning here. If we choose the 
 */
QgsRegionCorrelator::QgsRegionCorrelator(int szX, int szY, int usfac)
    : m_usfac(usfac)
{
    Q_ASSERT(szX > 1);
    Q_ASSERT(szY > 1);
    Q_ASSERT(usfac > 1);
    Q_ASSERT(usfac <= 1024); // larger than that is won't help much

    Q_ASSERT(sizeof(unsigned int) == 4);
    Q_ASSERT(sizeof(fftw_complex) == 16);
    Q_ASSERT(sizeof(gsl_complex) == 16);
    Q_ASSERT(sizeof(double) == 8);

    m_region_width = PowerOfTwoRoundUp(szX);
    m_region_height = PowerOfTwoRoundUp(szY);
       
    m_region1_data = (unsigned int *)fftw_malloc(m_region_width * m_region_height * sizeof(unsigned int));
    m_region2_data = (unsigned int *)fftw_malloc(m_region_width * m_region_height * sizeof(unsigned int));

    m_buf1ft = (fftw_complex *)fftw_malloc(m_region_width * m_region_height * sizeof(fftw_complex));
    m_buf2ft = (fftw_complex *)fftw_malloc(m_region_width * m_region_height * sizeof(fftw_complex));
    m_buftmp = (fftw_complex *)fftw_malloc(m_region_width * m_region_height * sizeof(fftw_complex));    

    // CC matrix is used in two places, so take the biggest size
    m_ccsize = std::max(2*m_region_width, int(ceil(usfac*1.5))) * std::max(2*m_region_height, int(ceil(usfac*1.5)));
    m_bufcc = (fftw_complex *)fftw_malloc(m_ccsize * sizeof(gsl_complex));
    
    const unsigned int flag = (1 ? FFTW_MEASURE : FFTW_ESTIMATE);
    m_plan1 = fftw_plan_dft_2d(m_region_width, m_region_height, m_buf1ft, m_buf1ft, FFTW_FORWARD, flag);
    m_plan2 = fftw_plan_dft_2d(m_region_width, m_region_height, m_buf2ft, m_buf2ft, FFTW_FORWARD, flag);
    m_plan3 = fftw_plan_dft_2d(2*m_region_width, 2*m_region_height, m_bufcc, m_bufcc, FFTW_BACKWARD, flag);
}

QgsRegionCorrelator::~QgsRegionCorrelator()
{
    fftw_destroy_plan(m_plan3);
    fftw_destroy_plan(m_plan2);
    fftw_destroy_plan(m_plan1);

    fftw_free(m_bufcc);
    fftw_free(m_buftmp);
    fftw_free(m_buf2ft);
    fftw_free(m_buf1ft);

    fftw_free(m_region2_data);
    fftw_free(m_region1_data);
}

/* ************************************************************************* */

/* 
 * This function calculates the offset between two regions by doing a 
 * correlation between the two arrays and finding the position with the
 * highest correlation value.
 */
void QgsRegionCorrelator::findRegion(int bits, double &colShift, double &rowShift, double *nrmsError, double *diffPhase)
{
    // References:
    // [1] Manuel Guizar-Sicairos, Samuel T. Thurman, and James R. Fienup, 
    //     "Efficient subpixel image registration algorithms", Optics Letters 33, 156-158 (2008).
    // http://www.optics.rochester.edu/~mguizar/Image_registration_OL_(2008).pdf
    // http://www.mathworks.com/matlabcentral/fileexchange/18401-efficient-subpixel-image-registration-by-cross-correlation
    // http://www.mathworks.com/matlabcentral/fx_files/18401/1/efficient_subpixel_registration.zip
    //
    gsl_complex *buf1ft = (gsl_complex *)m_buf1ft;
    gsl_complex *buf2ft = (gsl_complex *)m_buf2ft;

#if 0 // used for testing
    /* Copy regions into buffers as complex numbers */
    for (int i = 0; i < m_region_width * m_region_height; ++i)
    {
        GSL_SET_COMPLEX(&buf1ft[i], m_region1_data[i], 0.0);
        GSL_SET_COMPLEX(&buf2ft[i], m_region2_data[i], 0.0);
    }
#else
    /* Calculate the gradiant of the regions and copy into buffers */
    for (int i = 0; i < m_region_width * m_region_height; ++i)
    {
        m_region1_data[i] = bits - m_region1_data[i];
        m_region2_data[i] = bits - m_region2_data[i];
    }
    calculateGradient(buf1ft, m_region_width, m_region1_data, m_region_width, m_region_height, bits);
    calculateGradient(buf2ft, m_region_width, m_region2_data, m_region_width, m_region_height, bits);
#endif

    /* Calculate the fourier transform on 2D array. */
    fftw_execute(m_plan1);
    fftw_execute(m_plan2);

    // ---------------------------------------------------
    // MATLAB: 
    //  % First upsample by a factor of 2 to obtain initial estimate
    //  % Embed Fourier data in a 2x larger array
    //  [m,n] = size(buf1ft);
    //  CC = zeros(m*2,n*2);
    //  CC(m+1-fix(m/2):m+1+fix((m-1)/2),n+1-fix(n/2):n+1+fix((n-1)/2)) = fftshift(buf1ft) .* conj(fftshift(buf2ft));
    //  % Calculate cross-correlation
    //  CC = ifft2(ifftshift(CC)); 
    //  % Locate the peak 
    //  [max1,loc1] = max(CC);
    //  [max2,loc2] = max(max1);
    //  rloc=loc1(loc2); 
    //  cloc=loc2;
    //  CCmax=CC(rloc,cloc);
    //  m1 = m; n1 = n;
    //    
    const int usfac = m_usfac;
    const int m1 = m_region_width;
    const int n1 = m_region_height;
    const int m2 = m1*2;
    const int n2 = n1*2;

    gsl_complex *CC = (gsl_complex *)m_bufcc;    
    Q_ASSERT((m2*n2) <= m_ccsize);
    memset(CC, 0, m2*n2 * sizeof(gsl_complex));

    for (int r = 0; r < m1; ++r) 
    {
        int rr = r + (m2 - m1)/2;
        for (int c = 0; c < n1; ++c) 
        {           
            int cc = c + (n2 - n1)/2;
            int i = idx_fftshift(r,c, m1,n1);
            int j = idx_ifftshift(rr,cc, m2,n2);

            Q_ASSERT(0 <= i && i < (m1*m1));
            Q_ASSERT(0 <= j && j < (m2*m2));

            gsl_complex val = gsl_complex_mul(buf1ft[i], gsl_complex_conjugate(buf2ft[i]));
            
            double mag = gsl_complex_abs(val);            
            if (mag > DBL_EPSILON)
            {
                CC[j] = gsl_complex_div_real(val, mag);
            }
        }
    }

    //----------------------------------------------------
    int rloc, cloc;
    gsl_complex CCmax;

    // Inverse FFT operation
    fftw_execute(m_plan3);    

    // Find the location of the maximum real value in CC matrix
    gsl_complex_find_max(CC, m2, n2, rloc, cloc, CCmax);

    // Divide CCmax by m2*n2 due to the scaling done by fftw
    CCmax = gsl_complex_div_real(CCmax, m2*n2);

    // ---------------------------------------------------
    // MATLAB:
    //  % Obtain shift in original pixel grid from the position 
    //  % of the cross-correlation peak 
    //  [m2,n2] = size(CC);
    //  if rloc > m1 
    //      row_shift = rloc - m2 - 1;
    //  else
    //      row_shift = rloc - 1;
    //  end
    //  if cloc > n1
    //      col_shift = cloc - n2 - 1;
    //  else
    //      col_shift = cloc - 1;
    //  end
    //  row_shift=row_shift/2;
    //  col_shift=col_shift/2;
    double row_shift = (rloc - (rloc > m1 ? m2 : 0)) / 2.0;
    double col_shift = (cloc - (cloc > n1 ? n2 : 0)) / 2.0;

    // ---------------------------------------------------
    gsl_complex rg00, rf00;
    GSL_SET_COMPLEX(&rf00, 0,0);
    GSL_SET_COMPLEX(&rg00, 0,0);

    // If upsampling > 2, then refine estimate with matrix multiply DFT
    if (usfac > 2) 
    {
        // MATLAB:
        //  %%% DFT computation %%%
        //  % Initial shift estimate in upsampled grid
        //  row_shift = round(row_shift*usfac)/usfac; 
        //  col_shift = round(col_shift*usfac)/usfac;     
        //  % Center of output array at dftshift+1
        //  dftshift = fix(ceil(usfac*1.5)/2); 
        //
        row_shift = round(row_shift * usfac) / usfac;
        col_shift = round(col_shift * usfac) / usfac;
        int dftshift = fix(ceil(usfac*1.5)/2.0);

        // MATLAB: 
        //  % Matrix multiply DFT around the current shift estimate
        //  CC = conj(dftups(buf2ft.*conj(buf1ft),ceil(usfac*1.5),ceil(usfac*1.5),usfac,dftshift-row_shift*usfac,dftshift-col_shift*usfac))/(md2*nd2*usfac^2);
        //  % Locate maximum and map back to original pixel grid 
        //  [max1,loc1] = max(CC);   
        //  [max2,loc2] = max(max1); 
        //  rloc = loc1(loc2); cloc = loc2;
        //  CCmax = CC(rloc,cloc);
        //
        gsl_complex *buf = (gsl_complex *)m_buftmp;
        
        for (int i = 0; i < m1*n1; ++i) 
        {
            buf[i] = gsl_complex_mul(buf2ft[i], gsl_complex_conjugate(buf1ft[i]));
        }

        int mo = ceil(usfac*1.5);
        int no = ceil(usfac*1.5);
        Q_ASSERT((mo*no) <= m_ccsize);

        dftUpSample(buf, m1, n1, CC, mo, no, usfac, dftshift - row_shift*usfac, dftshift - col_shift*usfac);        
        
        for (int i = 0; i < mo*no; ++i)
        {
            CC[i] = gsl_complex_div_real(gsl_complex_conjugate(CC[i]), (m1*usfac)*(n1*usfac));
        }
        
        gsl_complex_find_max(CC, mo, no, rloc, cloc, CCmax);

        if (nrmsError)
        {
            // MATLAB:
            //  rg00 = dftups(buf1ft.*conj(buf1ft),1,1,usfac)/(m1*n1*usfac^2);
            //  rf00 = dftups(buf2ft.*conj(buf2ft),1,1,usfac)/(m1*n1*usfac^2);
            //  rloc = rloc - dftshift - 1;
            //  cloc = cloc - dftshift - 1;
            //  row_shift = row_shift + rloc/usfac;
            //  col_shift = col_shift + cloc/usfac;
            gsl_complex tmp;

            for (int i = 0; i < m1*n1; ++i) 
            {
                buf[i] = gsl_complex_mul(buf1ft[i], gsl_complex_conjugate(buf1ft[i]));
            }    
            dftUpSample(buf, m1, n1, &tmp, 1, 1, usfac);
            rg00 = gsl_complex_div_real(tmp, (m1*usfac)*(n1*usfac));
            
            for (int i = 0; i < m1*n1; ++i) 
            {
                buf[i] = gsl_complex_mul(buf2ft[i], gsl_complex_conjugate(buf2ft[i]));
            }
            dftUpSample(buf, m1, n1, &tmp, 1, 1, usfac);
            rf00 = gsl_complex_div_real(tmp, (m1*usfac)*(n1*usfac));
        }

        rloc -= dftshift;
        cloc -= dftshift;
        row_shift += double(rloc) / usfac;
        col_shift += double(cloc) / usfac;
    }
    else if (nrmsError)
    {
        // MATLAB:
        //  rg00 = sum(sum(buf1ft .* conj(buf1ft)))/(m1*n1);
        //  rf00 = sum(sum(buf2ft .* conj(buf2ft)))/(m1*n1);
        gsl_complex rg00_sum;
        gsl_complex rf00_sum;
        GSL_SET_COMPLEX(&rg00_sum, 0,0);
        GSL_SET_COMPLEX(&rf00_sum, 0,0);
        for (int i = 0; i < m1*n1; ++i) 
        {
            rg00_sum = gsl_complex_add(rg00_sum, 
                gsl_complex_mul(buf1ft[i], gsl_complex_conjugate(buf1ft[i])));
            rf00_sum = gsl_complex_add(rf00_sum, 
                gsl_complex_mul(buf2ft[i], gsl_complex_conjugate(buf2ft[i])));
        }
        rg00 = gsl_complex_div_real(rg00_sum, m1*n1);
        rf00 = gsl_complex_div_real(rf00_sum, m1*n1);
    }
    
    // MATLAB:
    //  error = 1.0 - CCmax.*conj(CCmax)/(rg00*rf00);
    //  error = sqrt(abs(error));
    //  diffphase=atan2(imag(CCmax),real(CCmax));
    //  % If its only one row or column the shift along that dimension has no
    //  % effect. We set to zero.
    //  if m1 == 1,
    //      row_shift = 0;
    //  end
    //  if n1 == 1,
    //      col_shift = 0;
    //  end
    //  output=[error,diffphase,row_shift,col_shift];
    //

    // Return the row shift, column shift, normalised RMS error and diff_phase values
    rowShift = row_shift;
    colShift = col_shift;
    if (nrmsError) 
    {
        gsl_complex one; 
        GSL_SET_COMPLEX(&one, 1.0, 0.0);
        gsl_complex e0 = gsl_complex_mul(CCmax, gsl_complex_conjugate(CCmax));
        gsl_complex e1 = gsl_complex_div(e0, gsl_complex_mul(rg00, rf00));
        gsl_complex e2 = gsl_complex_sub(one, e1);
        *nrmsError = sqrt(gsl_complex_abs(e2));
    }
    if (diffPhase)
    {
        *diffPhase = atan2(GSL_IMAG(CCmax), GSL_REAL(CCmax));
    }
}

/* ************************************************************************* */
#if 1 // Unit tests for find region function

#define API __declspec(dllexport) 

extern "C" API int QgsRegionCorrelator_test1();

static inline int sgn(double x) { return (x > 0.0) ? +1 : (x < 0.0) ? -1 : 0; }

API int QgsRegionCorrelator_test1()
{
    int errCount = 0;

    printf("Running QgsRegionCorrelator_test1 ...\n");

    /* Do a test with perfect pixel shift */
#define DO_TEST(dx,dy) do { \
        int offX = (dx); int offY = (dy); \
        for (int i = 0; i < w*h; ++i) p1[i] = p2[i] = 6; \
        int mX = w/2; int mY = h/2; \
        p1[mX+mY*w] = p2[(mX-offX)+(mY-offY)*w] = 12; \
        printf(" Results: off = (%2d,%2d)", offX,offY); \
        correlator.findRegion(255, shiftX, shiftY, &error, &diff_phase); \
        errCount += fabs(shiftX - offX) <= 0.05 ? 0 : 1; \
        errCount += fabs(shiftY - offY) <= 0.05 ? 0 : 1; \
        errCount += fabs(error) <= 0.00005 ? 0 : 1; \
        printf(", shift = (%6.3f,%6.3f), rmsErr = %7.4f, diff phase = %7.4f, e = %d\n", shiftX,shiftY, error, diff_phase, errCount); \
    } while(0)    

    /* Do a test with perfect half-pixel shift in x direction */
#define DO_TEST_HALF_X(dx,dy) do { \
        double offX = (dx); double offY = (dy); \
        for (int i = 0; i < w*h; ++i) p1[i] = p2[i] = 6; \
        int mX = w/2; int mY = h/2; int v = 106; double f = 0.5; \
        p1[mX+mY*w] = v; \
        p2[(mX-int(offX))+(mY-int(offY))*w] = 6 + (v-6)*(1.0 - f); \
        p2[(mX-int(offX)-sgn(offX))+(mY-int(offY))*w] = 6 + (v-6)*f; \
        printf(" Results: off = (%4.1f,%4.1f)", offX,offY); \
        correlator.findRegion(255, shiftX, shiftY, &error, &diff_phase); \
        errCount += fabs(shiftX - offX) <= 0.05 ? 0 : 1; \
        errCount += fabs(shiftY - offY) <= 0.05 ? 0 : 1; \
        printf(", shift = (%6.3f,%6.3f), rmsErr = %7.4f, diff phase = %7.4f, e = %d\n", shiftX,shiftY, error, diff_phase, errCount); \
    } while(0)

    /* Do a test with perfect half-pixel shift in y direction */
#define DO_TEST_HALF_Y(dx,dy) do { \
        double offX = (dx); double offY = (dy); \
        for (int i = 0; i < w*h; ++i) p1[i] = p2[i] = 6; \
        int mX = w/2; int mY = h/2; \
        p1[mX+mY*w] = 12; \
        p2[(mX-int(offX))+(mY-int(offY))*w] = 6 + (12-6)/2; \
        p2[(mX-int(offX))+(mY-int(offY)-sgn(offY))*w] = 6 + (12-6)/2; \
        printf(" Results: off = (%4.1f,%4.1f)", offX,offY); \
        correlator.findRegion(255, shiftX, shiftY, &error, &diff_phase); \
        errCount += fabs(shiftX - offX) <= 0.05 ? 0 : 1; \
        errCount += fabs(shiftY - offY) <= 0.05 ? 0 : 1; \
        printf(", shift = (%6.3f,%6.3f), rmsErr = %7.4f, diff phase = %7.4f, e = %d\n", shiftX,shiftY, error, diff_phase, errCount); \
    } while(0)

    QgsRegionCorrelator correlator(256, 256, 10);

    const int w = correlator.getRegionWidth();
    const int h = correlator.getRegionHeight();

    unsigned int *p1 = correlator.getRegion1();
    unsigned int *p2 = correlator.getRegion2();
    double shiftX, shiftY, error, diff_phase;

    DO_TEST(0,0);
    DO_TEST(+1,+2);
    DO_TEST(-4,+3);
    DO_TEST(-5,-7);
    DO_TEST(+6,-3);

    DO_TEST_HALF_X(-1.5,+0);
    DO_TEST_HALF_X(+7.5,+9);
    DO_TEST_HALF_Y(+5,+2.5);
    DO_TEST_HALF_Y(+9,-5.5);

    printf(" Error Count = %d\n", errCount);
    //printf("\nPress ENTER to continue.."); getchar();
#undef DO_TEST
    return errCount;
}

#endif
/* ************************************************************************* */
/* end of file */
