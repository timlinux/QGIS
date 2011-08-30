#include "qgscomplex.h"

QgsComplex::QgsComplex(double real, double imag)
{
  mReal = real;
  mImag = imag;
}
  
double QgsComplex::real()
{
  return mReal;
}

double QgsComplex::imag()
{
  return mImag;
}
  
void QgsComplex::setReal(double real)
{
  mReal = real;
}

void QgsComplex::setImag(double imag)
{
  mImag = imag;
}

QgsComplex QgsComplex::negative()
{
  QgsComplex c(-mReal, -mImag);
  return c;
}
  
QgsComplex QgsComplex::operator + (QgsComplex b)
{
  QgsComplex c(mReal + b.real(), mImag + b.imag());
  return c;
}

QgsComplex QgsComplex::operator - (QgsComplex b)
{
  QgsComplex c(mReal - b.real(), mImag - b.imag());
  return c;
}

QgsComplex QgsComplex::operator * (QgsComplex b)
{
  QgsComplex c(((mReal * b.real()) - (mImag * b.imag())), ((mReal * b.imag()) + (mImag * b.real())));
  return c;
}

QgsComplex QgsComplex::operator / (QgsComplex b)
{
  double div = (b.real() * b.real()) + (b.imag() * b.imag());
  if(div == 0.0)
  {
    return QgsComplex(0.0, 0.0);
  }
  QgsComplex c(((mReal * b.real()) + (mImag * b.imag())) / div, ((mImag * b.real()) - (mReal * b.imag())) / div);
  return c;
}

QgsComplex QgsComplex::operator + (double b)
{
  QgsComplex c(mReal + b, mImag);
  return c;
}

QgsComplex QgsComplex::operator - (double b)
{
  QgsComplex c(mReal - b, mImag);
  return c;
}

QgsComplex QgsComplex::operator * (double b)
{
  QgsComplex c((mReal * b), (mImag * b));
  return c;
}

QgsComplex QgsComplex::operator / (double b)
{
  if(b == 0.0)
  {
    return QgsComplex(0.0, 0.0);
  }
  double div = (b * b);
  QgsComplex c((mReal * b) / div, (mImag * b) / div);
  return c;
}