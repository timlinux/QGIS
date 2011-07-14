#include "qgsregioncorrelator.h"

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

QgsRegionCorrelationResult QgsRegionCorrelator::findRegion(QgsRegion *regionRef, QgsRegion *regionNew, int bits)
{  
  int counter = 0;
  int max = QgsRegionCorrelator::smallestPowerOfTwo(regionRef->width, regionNew->width);
  double *dataRef = new double[2*max*max];
  for(int i = 0; i < max*max*2; i+=2)
  {
    dataRef[i] = 0.0;
    dataRef[i+1] = 0.0;
  }

  double *dataNew = new double[2*max*max];
  for(int i = 0; i < max*max*2; i+=2)
  {
    dataNew[i] = 0.0;
    dataNew[i+1] = 0.0;
  }

  double *refD = QgsRegionCorrelator::caclculateGradient(regionRef->data, regionRef->width, bits);
  double *newD = QgsRegionCorrelator::caclculateGradient(regionNew->data, regionNew->width, bits);

  counter = 0;
  int counter2 = 0;
  for(int i = 0; i < regionRef->width; i++)
  {
    for(int j = 0; j < regionRef->height; j++)
    {
      REAL(dataRef, counter) = REAL(refD, counter2);
      IMAG(dataRef, counter) = IMAG(refD, counter2);
      counter2++;
      counter++;
    }
    counter += max-regionRef->width;
  }
  counter = 0;
  counter2 = 0;
  for(int i = 0; i < regionNew->width; i++)
  {
    for(int j = 0; j < regionNew->height; j++)
    {
      REAL(dataNew, counter) = REAL(newD, counter2);
      IMAG(dataNew, counter) = IMAG(newD, counter2);
      counter2++;
      counter++;
    }
    counter += max-regionNew->width;
  }
  QgsRegionCorrelator::fourierTransformNew(dataRef, max);
  QgsRegionCorrelator::fourierTransformNew(dataNew, max);
  gsl_complex dRef[max*max];
  gsl_complex dNew[max*max];
  counter = 0;
  for(int i = 0; i < max*max; i++)
  {
    gsl_complex g1;
    GSL_SET_COMPLEX(&g1, REAL(dataRef,i), IMAG(dataRef,i));
    dRef[counter] = g1;
    
    gsl_complex g2;
    GSL_SET_COMPLEX(&g2, REAL(dataNew,i), IMAG(dataNew,i));
    dNew[counter] = g2;
    
    counter++;
  }
  //Conjugate the complex numbers. Equivalent to numpy: conj()
  counter = 0;
  for(int i = 0; i < max*max; i++)
  {
    gsl_complex g2 = dNew[i];
    GSL_SET_IMAG(&g2, -1*GSL_IMAG(g2));
    dNew[counter] = g2;
    counter++;
  }

  //Multiply and calculate the absolute values for the result
  //Determine if any element evaluates to true. Equivalent to numpy: any()
  gsl_complex res[max*max];
  double resAbs[max*max];
  bool any = false;
  counter = 0;
  for(int i = 0; i < max*max; i++)
  {
     res[counter] = gsl_complex_mul(dRef[i], dNew[i]);
     resAbs[counter] = gsl_complex_abs(res[counter]);
     if(resAbs[counter])
     {
       any = true;
     }
     counter++;
  }

  if(any)
  {
    double conj[max*max*2];
    counter = 0;
    for(int i = 0; i < max*max*2; i+=2)
    {
      if(resAbs[counter] == 0 || resAbs[counter] != resAbs[counter])
      {
	conj[i] = 0;
	conj[i+1] = 0;
      }
      else
      {
	conj[i] = GSL_REAL(res[counter]) / resAbs[counter];
	conj[i+1] = GSL_IMAG(res[counter]) / resAbs[counter];
      }
      counter++;
    }

    QgsRegionCorrelator::inverseFourierTransformNew(conj, max);

    double *corr = QgsRegionCorrelator::inverseShift(conj, max);
    QList<QList<double> > interest = QgsRegionCorrelator::getSubset(corr, floor((max-regionRef->width)/2), floor((max+regionRef->width)/2), floor((max-regionRef->height)/2), floor((max+regionRef->height)/2), max);
    double theMax = LONG_MIN;
    QList<QList<int> > indexes;
    QList<QList<int> > values;
    for(int i = 0; i < interest.size(); i++)
    {
      QList<double> list = interest[i];
      for(int j = 0; j < list.size(); j++)
      {
	if(list[j] > theMax)
	{
	  theMax = list[j];
	  indexes.clear();
	}
	if(list[j] == theMax)
	{
	  QList<int> index;
	  index.append(i);
	  index.append(j);
	  indexes.append(index);
	}
      }
    }
    double y1 = indexes[0][0];
    double x1 = indexes[0][1];
    double y = y1-floor(regionRef->width/2.0)-1;
    double x = x1-floor(regionRef->height/2.0)-1;
    double deltaX;
    double deltaY;
    try
    {
      //Delta Y
      try
      {
	int signDelta = -1;
	deltaY = 0.0;
	if(interest.size() > y1+1 && interest.size() > y1-1 && y1-1 >= 0)
	{
	  if(interest[y1+1][x1] > interest[y1-1][x1])
	  {
	    signDelta = 1;
	  }
	  if(interest.size() > y1+signDelta && interest[y1+signDelta][x1] == theMax)
	  {
	    deltaY = 0.5 * signDelta;
	  }
	  else
	  {
	    double a = interest[y1+signDelta][x1] / double(interest[y1+signDelta][x1] - theMax);
	    double b = interest[y1+signDelta][x1] / double(interest[y1+signDelta][x1] + theMax);
	    if(a*signDelta > 0 && a*signDelta < 0.5)
	    {
	      deltaY = a;
	    }
	    else if(b*signDelta > 0 && a*signDelta < 0.5)
	    {
	      deltaY = b;
	    }
	    else
	    {
	      deltaY = min(fabs(a),fabs(b)) * signDelta;
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
	if(interest.size() > y1 && interest[y1].size() > x1+1 && x1-1 >= 0)
	{
	  if(interest[y1][x1+1] > interest[y1][x1-1])
	  {
	    signDelta = 1;
	  }
	  if(interest[y1].size() > x1+signDelta && interest[y1][x1+signDelta] == theMax)
	  {
	    deltaX = 0.5 * signDelta;
	  }
	  else
	  {
	    double a = interest[y1][x1+signDelta] / double(interest[y1][x1+signDelta] - theMax);
	    double b = interest[y1][x1+signDelta] / double(interest[y1][x1+signDelta] + theMax);
	    if(a*signDelta > 0 && a*signDelta < 0.5)
	    {
	      deltaX = a;
	    }
	    else if(b*signDelta > 0 && a*signDelta < 0.5)
	    {
	      deltaX = b;
	    }
	    else
	    {
	      deltaX = min(fabs(a),fabs(b)) * signDelta;
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
    double snr = 0;//QgsRegionCorrelator::quantifySnr(interest, x1, y1);
    double confidence = 0;//QgsRegionCorrelator::quantifyConfidence(interest);
    
    delete [] dataRef;
    delete [] dataNew;
    delete [] refD;
    delete [] newD;
    return QgsRegionCorrelationResult(true, x, y, max, theMax, confidence, snr);
  }
  else
  {
    delete [] dataRef;
    delete [] dataNew;
    delete [] refD;
    delete [] newD;
    return QgsRegionCorrelationResult(false);
  }
}

double* QgsRegionCorrelator::caclculateGradient(uint *array, int dimension, int bits, double order)
{
  //Re-scaled all values to to 0 - 65535 (eg: -66 = 65535 -66)
  int scale = bits;
  double dx = order;
  double *x = new double[dimension*dimension];
  for(int i = 0; i < dimension; i++)
  {
    bool neg = false;
    int val = array[i*dimension + 1] - array[i*dimension];
    if(val < 0)
    {
      val = scale + val;
    }
    x[i*dimension] = floor(val / dx);
    val = array[i*dimension+dimension-1] - array[i*dimension+dimension-2];
    if(val < 0)
    {
      neg = true;
      val = scale + val;
    }
    if(neg) x[i*dimension+dimension-1] = floor(val / dx)+1;
    else x[i*dimension+dimension-1] = floor(val / dx);
    
    for(int j = 1; j < dimension-1; j++)
    {
      val = array[i*dimension+j+1] - array[i*dimension+j-1];
      neg = false;
      if(val < 0)
      {
	neg = true;
	val = scale - val;
      }
      if(neg) x[i*dimension+j] = floor(val / (dx*2.0))-1;
      else x[i*dimension+j] = floor(val / (dx*2.0));
    }
  }

  double dy = order;
  double *y = new double[dimension*dimension];
  for(int i = 0; i < dimension; i++)
  {
    int val = array[dimension + i] - array[i];
    if(val < 0)
    {
      val = scale + val;
    }
    y[i] = floor(val / dx);
    val = array[dimension*(dimension-1) + i] - array[(dimension*(dimension-1) - dimension) + i];
    if(val < 0)
    {
      val = scale - val;
    }
    y[dimension*(dimension-1) + i] = floor(val / dx);
    for(int j = 1; j < dimension-1; j++)
    {
      val = array[j*dimension + dimension + i] - array[j*dimension - dimension + i];
      if(val < 0)
      {
	val = scale + val;
      }
      y[j*dimension+i] = floor(val / (dy*2));
    }
  } 
  
  double *result = new double[dimension*dimension*2];
  int to = dimension*dimension*2;
  int counter = 0;
  for(int i = 0; i < to; i+=2)
  {
    result[i] = x[counter];
    result[i+1] = y[counter];
    counter++;
  }
    
  delete [] x;
  delete [] y;
  
  return result;
}

int QgsRegionCorrelator::smallestPowerOfTwo(int value1, int value2)
{
  if(sqrt(value1) == int(sqrt(value1)))
  {
    return value1;
  }
  int n = max(value1, value2);
  int result = floor(2*ceil(log(n)/log(2)));
  return result*result;
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
  int counter = 0;
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
  int rows = toRow-fromRow;
  int cols = toCol-fromCol;
  QList<QList<double> > result;
  int counter = 0;
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
    double *col = new double[size*2];
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
    array[i] = in[counter][0]/(size*size); //Devide my size*size due to the scaling done by fftw
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