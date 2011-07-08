#ifndef QGSCOMPLEX_H
#define QGSCOMPLEX_H

class QgsComplex
{
  public:
    
  QgsComplex(double real = 0.0, double imag = 0.0);
  
  double real();
  double imag();
  
  void setReal(double real);
  void setImag(double imag);
  
  QgsComplex negative();
  
  QgsComplex operator + (QgsComplex b);
  QgsComplex operator - (QgsComplex b);
  QgsComplex operator * (QgsComplex b);
  QgsComplex operator / (QgsComplex b);
  
  QgsComplex operator + (double b);
  QgsComplex operator - (double b);
  QgsComplex operator * (double b);
  QgsComplex operator / (double b);

  private:
    
    double mReal;
    double mImag;
    
};

#endif