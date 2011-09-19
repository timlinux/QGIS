#include "qgsregioncorrelator.h"

#include <QTime>

// Round up to next higher power of 2 (return x if it's already a power of 2).
// Reference: http://aggregate.org/MAGIC/#Next%20Largest%20Power%20of%202
inline unsigned int PowerOfTwoRoundUp(unsigned int x) 
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return (x+1);
}

inline unsigned int NextIntegerSquare(unsigned int x)
{
    unsigned result = ceil(sqrt((double)x));
    return result * result;
}

void circshift(double *out, const double *in, int xdim, int ydim, int xshift, int yshift)
{
  for(int i =0; i < xdim; i++)
  {
    int ii = (i + xshift) % xdim;
    for(int j = 0; j < ydim; j++)
    {
      int jj = (j + yshift) % ydim;
      out[ii * ydim + jj] = in[i * ydim + j];
    }
  }
}

QgsRegionCorrelator::QgsRegionCorrelator()
{
  
}

QgsRegionCorrelationResult QgsRegionCorrelator::findRegion(QgsRegion &regionRef, QgsRegion &regionNew, int bits)
{  
    int maxW = PowerOfTwoRoundUp(std::max(regionRef.width, regionNew.width));
    int maxH = PowerOfTwoRoundUp(std::max(regionRef.height, regionNew.height));
    int max = std::max(maxW,maxH);

//  FILE *f = stdout;
//  int sectionRunTime;
//  QTime profileTimer;
//  fprintf(f, "findRegion: ");
//  profileTimer.start();

    gsl_complex *dataRef = (gsl_complex *)fftw_malloc(max * max * sizeof(gsl_complex));
    gsl_complex *dataNew = (gsl_complex *)fftw_malloc(max * max * sizeof(gsl_complex));

//  fprintf(f, " %d", profileTimer.elapsed());
//  profileTimer.start();

    QgsRegionCorrelator::calculateGradient(dataRef, max, regionRef.data, regionRef.width, regionRef.height, bits);
    QgsRegionCorrelator::calculateGradient(dataNew, max, regionNew.data, regionNew.width, regionNew.height, bits);

//  fprintf(f, " %d", profileTimer.elapsed());

    fftw_plan plan;  
  
//  profileTimer.start();

    plan = fftw_plan_dft_2d(max, max, (fftw_complex*)dataRef, (fftw_complex*)dataRef, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(plan);
    fftw_destroy_plan(plan);

    plan = fftw_plan_dft_2d(max, max, (fftw_complex*)dataNew, (fftw_complex*)dataNew, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(plan);
    fftw_destroy_plan(plan);

//  fprintf(f, " %d", profileTimer.elapsed());

    gsl_complex *conj = (gsl_complex *)fftw_malloc(max * max * sizeof(gsl_complex));
    bool any = false;

//  profileTimer.start();

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
            any = true;
        } else {
            GSL_SET_COMPLEX(&conj[i], 0, 0);
        }
    }

//  fprintf(f, " %d  X ", profileTimer.elapsed());

    fftw_free(dataNew);
    fftw_free(dataRef);

    //----------------------------------------------------
    // Optimization: quick exist if whole conj matrix is zeros
    if (any == false) 
    {
        return QgsRegionCorrelationResult(false);
    }
    //----------------------------------------------------


//  profileTimer.start();

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

//  fprintf(f, " IFFT %d", profileTimer.elapsed());

    gsl_complex *corr = (gsl_complex *)QgsRegionCorrelator::ifftShiftComplex((double*)conj, max);
    
    // Create an interest region in the middle of conj area

    int fromRow = (max - regionRef.width) / 2;
    int toRow   = (max + regionRef.width) / 2;
    int fromCol = (max - regionRef.height) / 2;
    int toCol   = (max + regionRef.height) / 2;

    gsl_complex *interest = &corr[max * fromRow + fromCol];
    
    // Find the index(es) with the maximum value

    double theMax = -DBL_MAX;
    QList<std::pair<int,int>> indexes;

    for (int r = 0; r < (toRow - fromRow); ++r)
    {
        for (int c = 0; c < (toCol - fromCol); ++c)
        {
            double value = GSL_REAL(interest[max * r + c]);

            if (theMax < value) 
            {
                theMax = value;
                indexes.clear();
            }             
            if (theMax == value) 
            {
                indexes.append(make_pair(r,c));
            }
        }
    }

    Q_ASSERT(indexes.size() > 0);
    Q_ASSERT(theMax != -DBL_MAX);
      
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

                if (ySize > (y1+signDelta) && value == theMax)
                {
                    deltaY = 0.5 * signDelta;
                }
                else
                {
                    double a = value / (value - theMax);
                    double b = value / (value + theMax);

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

                if (xSize > x1+signDelta && value == theMax) 
                {
                    deltaX = 0.5 * signDelta;
                }
                else
                {
                    double a = value / (value - theMax);
                    double b = value / (value + theMax);

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

#if 0
    //double snr = QgsRegionCorrelator::quantifySnr(interest, x1, y1);
    double snr = QgsRegionCorrelator::quantifySNR(interest, xSize, x1, y1);
    double confidence = 0.0; //QgsRegionCorrelator::quantifyConfidence(interest);
#else
    double snr = 0;
    double confidence = 0;
#endif

    fftw_free(conj);

    return QgsRegionCorrelationResult(true, x, y, max, theMax, confidence, snr);
}

gsl_complex *QgsRegionCorrelator::calculateGradient(gsl_complex *buffer, int rowStride,
        uint *data, int width, int height, int bits, double order)
{
    Q_CHECK_PTR(buffer);
    Q_ASSERT(rowStride > 0);
    Q_CHECK_PTR(data);

#define IDX(r,c) ((r)*width + (c))

    // Re-scaled all values to to 0 - 65535 (eg: -66 = 65535 -66)
    int scale = bits;

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
#undef IDX_IMAG
#undef IDX_REAL
#undef IDX_STRIDE
}

#if 0
double* QgsRegionCorrelator::calculateGradient(uint *data, int width, int height, int bits, double order)
{
    Q_ASSERT(width > 0);
    Q_ASSERT(height > 0);
    double *buf = new double[2*width*height];
    return calculateGradient(buf, width, data, width, height, bits, order);
}
#endif

int QgsRegionCorrelator::smallestPowerOfTwo(int value1, int value2)
{
  if(sqrt((double)value1) == int(sqrt((double)value1)))
  {
    return value1;
  }
  int n = max(value1, value2);
  int result = floor(2.0*ceil(log((double)n)/log(2.0)));
  return result*result;
}

double* QgsRegionCorrelator::ifftShiftComplex(double *data, int dimension)
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

double* QgsRegionCorrelator::inverseShift(double *data, int dimension)
{
    //Create 4 blocks
    double *block1 = new double[dimension*dimension/2];
    double *block2 = new double[dimension*dimension/2];
    double *block3 = new double[dimension*dimension/2];
    double *block4 = new double[dimension*dimension/2];
    int counter1 = 0;
    int counter2 = 0;
    int counter3 = 0;
    int counter4 = 0;
    for(int i = 0; i < dimension; i++)
    {
        for(int j = 0; j < dimension*dimension/2; j+=2)
        {
            if(i<dimension/2 && j<dimension)//Block 1
            {
                block1[counter1] = data[i*dimension*2 + j];
                block1[counter1+1] = data[i*dimension*2 + j + 1];
                counter1 += 2;
            }
            else if(i<dimension/2 && j<dimension*2)//Block 2
            {
                block2[counter2] = data[i*dimension*2 + j];
                block2[counter2+1] = data[i*dimension*2 + j + 1];
                counter2 += 2;
            }
            else if(i<dimension && j<dimension)//Block 3
            {
                block3[counter3] = data[i*dimension*2 + j];
                block3[counter3+1] = data[i*dimension*2 + j + 1];
                counter3 += 2;
            }
            else if(i<dimension && j<dimension*2)//Block 4
            {
                block4[counter4] = data[i*dimension*2 + j];
                block4[counter4+1] = data[i*dimension*2 + j + 1];
                counter4 += 2;
            }
        }
    }

    counter1 = 0;
    counter2 = 0;
    counter3 = 0;
    counter4 = 0;
    for(int i = 0; i < dimension; i++)
    {
        for(int j = 0; j < dimension*dimension/2; j+=2)
        {
            if(i<dimension/2 && j<dimension)//Block 1
            {
                data[i*dimension*2 + j] = block4[counter4];
                data[i*dimension*2 + j + 1] = block4[counter4+1];
                counter4 += 2;
            }
            else if(i<dimension/2 && j<dimension*2)//Block 2
            {
                data[i*dimension*2 + j] = block3[counter3];
                data[i*dimension*2 + j + 1] = block3[counter3+1];
                counter3 += 2;
            }
            else if(i<dimension && j<dimension)//Block 3
            {
                data[i*dimension*2 + j] = block2[counter2];
                data[i*dimension*2 + j + 1] = block2[counter2+1];
                counter2 += 2;
            }
            else if(i<dimension && j<dimension*2)//Block 4
            {
                data[i*dimension*2 + j] = block1[counter1];
                data[i*dimension*2 + j + 1] = block1[counter1+1];
                counter1 += 2;
            }
        }

    }

    delete [] block1;
    delete [] block2;
    delete [] block3;
    delete [] block4;

    return data;
}

QList<QList<double> > QgsRegionCorrelator::getSubset(double *data, int fromRow, int toRow, int fromCol, int toCol, int dimension)
{
  QList<QList<double> > result;
  //int counter = 0;
  for(int i = fromRow; i < toRow; i++)
  {
    QList<double> list;
    for(int j = fromCol; j < toCol; j++)
    {
      list.append(data[i*dimension*2+j*2]);
      //Add the next line to add the imag to the list as well
      //list.append(data[i*dimension + j+1]);
    }
    result.append(list);
  }
  return result;
}

QList<QList<double> > QgsRegionCorrelator::getQListSubset(QList<QList<double> > data, int fromRow, int toRow, int fromCol, int toCol)
{
  //int rows = toRow-fromRow;
  //int cols = toCol-fromCol;
  QList<QList<double> > result;
  //int counter = 0;
  for(int i = fromRow; i < toRow; i++)
  {
    QList<double> list;
    for(int j = fromCol; j < toCol; j++)
    {
      list.append(data[i][j]);
    }
    result.append(list);
  }
  return result;
}

void QgsRegionCorrelator::fourierTransform(double *array, int size)
{
  int counter;
  QList<double*> rows;
  for(int i = 0; i < size; i++)
  {
    double *row = new double[size*2];
    for(int j = 0; j < size*2; j+=2)
    {
      row[j] = array[i*(size*2) + j];
      row[j + 1] = array[i*(size*2) + j + 1];
    }
    rows.append(row);
  }
  
  for(int i = 0; i < rows.size(); i++)
  {
    gsl_fft_complex_radix2_forward(rows[i], 1, size);
  }

  QList<double*> cols;
  for(int i = 0; i < size*2; i+=2)
  {
    double *col = new double[size*2];
    counter = 0;
    for(int j = 0; j < rows.size(); j++)
    {
      col[counter] = rows[j][i];
      col[counter+1] = rows[j][i+1];
      counter+=2;
    }
    cols.append(col);
  }
  
  for(int i = 0; i < cols.size(); i++)
  {
    gsl_fft_complex_radix2_forward(cols[i], 1, size);
  }
  
  counter = 0;
  for(int i = 0; i < size*2; i+=2)
  {
    //double *col = new double[size*2];
    for(int j = 0; j < cols.size(); j++)
    {
      array[counter] = cols[j][i];
      array[counter+1] = cols[j][i+1];
      counter+=2;
    }
  }
  
  for(int j = 0; j < rows.size(); j++)
  {
    delete [] rows[j];
  }
  for(int j = 0; j < cols.size(); j++)
  {
    delete [] cols[j];
  }
}

void QgsRegionCorrelator::fourierTransformNew(double *array, int size)
{
  fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size * size);
  int counter = 0;
  for(int i = 0; i < size*size*2; i+=2)
  {
    in[counter][0] = array[i];
    in[counter][1] = array[i+1];
    counter++;
  }
  fftw_plan plan = fftw_plan_dft_2d(size, size, in, in, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(plan);
  counter = 0;
  for(int i = 0; i < size*size*2; i+=2)
  {
    array[i] = in[counter][0];
    array[i+1] = in[counter][1];
    counter++;
  }
  fftw_free(in);
  fftw_destroy_plan(plan);
  //fftw_cleanup();
}

void QgsRegionCorrelator::inverseFourierTransformNew(double *array, int size)
{
  fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size * size);
  int counter = 0;
  for(int i = 0; i < size*size*2; i+=2)
  {
    in[counter][0] = array[i];
    in[counter][1] = array[i+1];
    counter++;
  }
  
  fftw_plan plan=fftw_plan_dft_2d(size, size, in, in, FFTW_BACKWARD, FFTW_ESTIMATE);
  fftw_execute(plan);
  
  counter = 0;
  for(int i = 0; i < size*size*2; i+=2)
  {
    array[i] = in[counter][0]/(size*size); //Divide by size*size due to the scaling done by fftw
    array[i+1] = in[counter][1]/(size*size);
    counter++;
  }
  fftw_destroy_plan(plan);
  fftw_free(in);
  //fftw_cleanup();
}

void QgsRegionCorrelator::inverseFourierTransform(double *array, int size)
{
  int counter;
  QList<double*> rows;
  for(int i = 0; i < size; i++)
  {
    double *row = new double[size*2];
    for(int j = 0; j < size*2; j+=2)
    {
      row[j] = array[i*(size*2) + j];
      row[j + 1] = array[i*(size*2) + j + 1];
    }
    rows.append(row);
  }
  
  for(int i = 0; i < rows.size(); i++)
  {
    gsl_fft_complex_radix2_inverse(rows[i], 1, size);
  }

  QList<double*> cols;
  for(int i = 0; i < size*2; i+=2)
  {
    double *col = new double[size*2];
    counter = 0;
    for(int j = 0; j < rows.size(); j++)
    {
      col[counter] = rows[j][i];
      col[counter+1] = rows[j][i+1];
      counter+=2;
    }
    cols.append(col);
  }
  
  for(int i = 0; i < cols.size(); i++)
  {
    gsl_fft_complex_radix2_inverse(cols[i], 1, size);
  }

  counter = 0;
  for(int i = 0; i < size*2; i+=2)
  {
    for(int j = 0; j < cols.size(); j++)
    {
      array[counter] = cols[j][i];
      array[counter+1] = cols[j][i+1];
      counter+=2;
    }
  }
  for(int j = 0; j < cols.size(); j++)
  {
    delete [] cols[j];
  }
  for(int j = 0; j < rows.size(); j++)
  {
    delete [] rows[j];
  }
}

void QgsRegionCorrelator::print(double *array, int size)
{
  cout<<"**ARRAY>>*************************"<<endl;
  int counter = 0;
  for(int i =0; i < size*size*2; i+=2)
  {
    cout<<array[i]<<"+"<<array[i+1]<<"j  ";
    counter++;
    if(counter%4 == 0) cout<<endl;
  }
  cout<<"**ARRAY<<*************************"<<endl;
}

double QgsRegionCorrelator::quantifySNR(gsl_complex *data, int dim, double x, double y)
{
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


double QgsRegionCorrelator::quantifySnr(QList<QList<double> > array, double x, double y)
{
  QList<QList<double> > sub = QgsRegionCorrelator::getQListSubset(array, y-1, y+2, x-1, x+2);
  double energySignal = 0.0;
  for(int i = 0; i < sub.size(); i++)
  {
    QList<double> list = sub[i];
    for(int j = 0; j < list.size(); j++)
    {
      energySignal += pow(list[j], 2);
    }
  }
  double energyNoise = 0.0;
  for(int i = 0; i < array.size(); i++)
  {
    QList<double> list = array[i];
    for(int j = 0; j < list.size(); j++)
    {
      energyNoise += pow(list[j], 2);
    }
  }
  energyNoise -= energySignal;
  if(energyNoise)
  {
    if(energySignal)
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

double QgsRegionCorrelator::quantifyConfidence(QList<QList<double> > array)
{/*
  int points = array.size() * array[0].size();
		arrayc = detect_local_maxima(array)*array
		arrayc = np.reshape(arrayc,(1,Points))
		arrayc.sort()
		conf = arrayc[0][-1]/arrayc[0][-2]
		return 1-1/conf*/
  return 0.0;
}