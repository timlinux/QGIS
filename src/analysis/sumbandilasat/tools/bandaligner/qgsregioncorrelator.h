#ifndef QGSREGIONCORRELATOR_H
#define QGSREGIONCORRELATOR_H

#define REAL(z,i) ((z)[2*(i)])
#define IMAG(z,i) ((z)[2*(i)+1])

#define fftshift(out, in, x, y) circshift(out, in, x, y, (x/2), (y/2))

#include <fftw3.h>


#include <gsl/gsl_fft_complex.h>
#include <gsl/gsl_complex_math.h>

#include <math.h>
#include <limits.h>
#include <iostream>

#include <QString>
#include <QList>

#define round(val) (floor(val))

using namespace std;

struct QgsRegionCorrelationResult
{
  QgsRegionCorrelationResult(bool a, double b = -1, double c = -1, int d = -1, double e = -1, double f = -1, double g = -1)
  {
    wasSet = a;
    x = b;
    y = c;
    dimension = d;
    match = e;
    confidence = f;
    snr = g;
  }
  bool wasSet;
  double x;
  double y;
  int dimension;
  double match;
  double confidence;
  double snr;
};

struct QgsRegion
{
  uint *data;
  int width;
  int height;
};

void circshift(double *out, const double *in, int xdim, int ydim, int xshift, int yshift);

class ANALYSIS_EXPORT QgsRegionCorrelator
{
  public:
    
  QgsRegionCorrelator();
  static QgsRegionCorrelationResult findRegion(QgsRegion &regionRef, QgsRegion &regionNew, int bits);
  
  protected:
    /*
    Calculate the fourier transform on 2D array.
    size = width (or height) of 2D array. Width must be equal to height
    */
    static void fourierTransform(double *array, int size);
    
    static void fourierTransformNew(double *array, int size);
    
    /*
    Calculate the inverse fourier transform on 2D array.
    size = width (or height) of 2D array. Width must be equal to height
    */
    static void inverseFourierTransform(double *array, int size);
    
    static void inverseFourierTransformNew(double *array, int size);
    
    /*
    Print array were i == real and i+1 == imag
    */
    static void print(double *array, int size);
    
    /*
    Calculate the gradient of the array
    */
    //static double* calculateGradient(uint *data, int width, int height, int bits, double order = 2.0);

    static gsl_complex* calculateGradient(gsl_complex *buffer, int rowStride, uint *data, int width, int height,int bits, double order = 2.0);

    /*
    Find the smallest power of 2 bigger than the biggest of size1 and size2 dimentions
    */
    static int smallestPowerOfTwo(int value1, int value2);
    
    /*
    Inverse shift
    */
    static double* inverseShift(double *data, int dimension);
    static double* ifftShiftComplex(double *data, int dimension);
    
    /*
    Cut out a certain area of the array
    */
    static QList<QList<double> > getSubset(double *data, int fromRow, int toRow, int fromCol, int toCol, int dimension);
    
    /*
    Cut out a certain area of the QList
    */
    static QList<QList<double> > getQListSubset(QList<QList<double> > data, int fromRow, int toRow, int fromCol, int toCol);
    
    static double quantifySNR(gsl_complex *data, int dim, double x, double y);

    static double quantifySnr(QList<QList<double> > array, double x, double y);
    
    static double quantifyConfidence(QList<QList<double> > array);
    
  
  private:

};

#endif