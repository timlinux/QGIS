/***************************************************************************
qgsmatrix.cpp - A two dimensional m x n matrix
--------------------------------------
Date : 14-September-2010
Author : James Meyer
Copyright : (C) 2010 by C.J. Meyer
Email : jamesmeyerx@gmail.com

/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/

#include "qgsmatrix.h"
#include "qgslogger.h"
#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <vector>

QgsMatrix::QgsMatrix( int rows, int cols, bool initialize )
    : mCols( cols ), mRows( rows ), mData( NULL )
{

  //Initialize matrix data
  resize( mRows, mCols , initialize );
}

QgsMatrix::QgsMatrix( const QgsMatrix& other )
    : mCols( other.mCols ), mRows( other.mRows ), mData( NULL )
{
  //Initialize matrix data
  resize( mCols, mRows );
  //Copy other's data
  int elements = mRows * mCols;
  if ( other.mData )
  {
    for ( int i = 0; i < elements; ++i )
    {
      mData[i] = other.mData[i];
    }
  }
}
QgsMatrix::~QgsMatrix()
{
  if ( mData )
  {
    delete mData;
  }
}
const QgsMatrix& QgsMatrix::operator=( const QgsMatrix & other )
{
  if ( other.mData && rows() == other.rows() && columns() == other.columns() )
  {
    int elements = rows() * columns();
    for ( int i = 0; i < elements; ++i )
    {
      mData[i] = other.mData[i];
    }
  }
  return *this;
}

void QgsMatrix::resize( int rows, int cols, bool initialize )
{
  if ( cols > 0 && rows > 0 )
  {
    int elements = cols * rows;
    double* tempData = new double[elements];
    if ( initialize )
    {
      for ( int i = 0; i < elements; i++ )
      {
        tempData[i] = 0.0;
      }
    }
    if ( mData )
    {
      int minCols = std::min( mCols, cols );
      int minRows = std::min( mRows, rows );
      for ( int row = 0; row < minRows; ++row )
      {
        for ( int col = 0; col < minCols; ++col )
        {
          tempData[row * cols + col] = ( *this )( row, col );
        }
      }
    }
    delete mData;
    mData = tempData;

  }
}

QgsMatrix QgsMatrix::transpose() const
{
  //Create temp matrix with flipped dimensions
  QgsMatrix m( mCols, mRows, false );
  QgsMatrix::transpose( &m, this );
  return m;
}

QgsMatrix QgsMatrix::inverse() const
{
  QgsMatrix m( mRows, mCols, false );
  QgsMatrix::invert( &m, this );
  return m;
}

bool QgsMatrix::isIdentity() const
{
  if ( rows() != columns() )
  {
    return false;
  }
  for ( int i = 0; i < rows(); ++i )
  {
    for ( int j = 0; j < columns(); ++j )
    {
      if ( i == j )
      {
        if ( !FLT_EQUALS(( *this )( i, j ), 1.0 ) )
        {
          return false;
        }
      }
      else if ( !FLT_ZERO(( *this )( i, j ) ) )
      {
        return false;
      }
    }
  }
  return true;
}


//************************************************************************
// STATIC MATRIX METHODS
//************************************************************************
QgsMatrix* QgsMatrix::identity( QgsMatrix* out )
{
  if ( out->columns() == out->rows() )
  {
    for ( int i = 0; i < out->rows(); i++ )
    {
      for ( int j = 0; j < out->columns(); j++ )
      {
        if ( i == j )
        {
          ( *out )( i, j ) = 1.0;
        }
        else
        {
          ( *out )( i, j ) = 0.0;
        }
      }
    }
    return out;
  }
  else
  {
    return NULL;
  }

}

QgsMatrix* QgsMatrix::multiply( QgsMatrix* out, const QgsMatrix* left, const QgsMatrix* right )
{
  if ( left->columns() != right->rows() || out->columns() != right->columns() || out->rows() != left->rows() )
  {
    return NULL;
  }
  //Make a temporary copy for output, incase left == out || right ==out
  QgsMatrix m( out->rows(), out->columns(), false );
  const QgsMatrix& l = *left;
  const QgsMatrix& r = *right;
  int range = l.mCols;//same as right->mRows
  for ( int i = 0; i < m.rows(); ++i )
  {
    for ( int j = 0; j < m.columns(); j++ )
    {
      double value = 0.0;
      for ( int k = 0; k < range; ++k )
      {
        value += l( i, k ) * r( k, j );
      }
      //printf("%d,%d = %f\n",i,j,value);
      m( i, j ) = value;
    }
  }
  *out = m;
  return out;
}
QgsMatrix* QgsMatrix::add( QgsMatrix* out, const QgsMatrix* left, const QgsMatrix* right )
{
  int cols = left->columns();
  int rows =  left->rows();
  if ( right->columns() == cols && right->rows() == rows )
  {
    int elements = rows * cols;
    for ( int i = 0; i < elements; ++i )
    {
      out->mData[i] = left->mData[i] + right->mData[i];
    }
  }
  else
  {
    return NULL;
  }
  return out;
}
QgsMatrix* QgsMatrix::subtract( QgsMatrix* out, const QgsMatrix* left, const QgsMatrix* right )
{
  int cols = left->columns();
  int rows =  left->rows();
  if ( right->columns() == cols && right->rows() == rows )
  {
    int elements = rows * cols;
    for ( int i = 0; i < elements; ++i )
    {
      out->mData[i] = left->mData[i] - right->mData[i];
    }
  }
  else
  {
    return NULL;
  }
  return out;
}

QgsMatrix* QgsMatrix::multiply( QgsMatrix* out, const QgsMatrix* in, double value )
{
  for ( int i = 0; i < out->rows(); ++i )
  {
    for ( int j = 0; j < out->columns(); ++j )
    {
      out->setElement( i, j, ( *in )( i, j ) * value );
    }
  }
  return out;
}
QgsMatrix* QgsMatrix::divide( QgsMatrix* out, const QgsMatrix* in, double value )
{
  if ( value == 0.0 )
  {
    return out;
  }
  for ( int i = 0; i < out->rows(); ++i )
  {
    for ( int j = 0; j < out->columns(); ++j )
    {
      out->setElement( i, j, ( *in )( i, j ) / value );
    }
  }
  return out;
}
QgsMatrix* QgsMatrix::transpose( QgsMatrix* out, const QgsMatrix* in )
{
  int rows = out->rows(), cols = out->columns();
  if ( in->columns() == rows && in->rows() == cols )
  {
    QgsMatrix input( *in );//Copy case out == in
    QgsMatrix& output = *out;
    for ( int i = 0; i < rows; ++i )
    {
      for ( int j = 0; j < cols; ++j )
      {
        output( i, j ) = input( j, i );
      }
    }
  }
  else
  {
    return NULL;
  }
  return out;

}
QgsMatrix* QgsMatrix::invert( QgsMatrix* out, const QgsMatrix* in )
{
  return NULL;
}

QgsMatrix* QgsMatrix::solve( QgsMatrix* X, const QgsMatrix* A, const QgsMatrix* B )
{
  //check that A is square...
  if ( A->rows() != A->columns() ) return NULL;
  //check that X->columns() == B->columns()
  if ( X->columns() != B->columns() ) return NULL;
  for ( int i = 0; i < B->columns(); i++ )
  {
    if ( NULL == solveVector( X, A, B, i ) )
    {
      QgsLogger::debug( "Could not solve vector." );
      return NULL;
    }
  }
  return X;
}

QgsMatrix* QgsMatrix::solveVector( QgsMatrix* X, const QgsMatrix* A, const QgsMatrix* B, int col )
{


  /*

  int row,xcol,row_size,col_size;
    float data,sum;
    matrix_t lower,upper,perm,matrix_x,matrix_y,matrix_pb;

    row_size=A->rows();
    col_size=A.columns();

    matrix_build_LU2(matrix_a,&lower,&upper);

    perm=matrix_build_P(matrix_a);
    matrix_x=matrix_build_zero(row_size,1);
    matrix_y=matrix_build_zero(row_size,1);

    matrix_pb=matrix_mul(perm,matrix_b);

    for(row=0;row<row_size;row++){
        sum=0;
        for(xcol=0;xcol<row;xcol++){
            sum+=matrix_get_data(lower,row,xcol)*matrix_get_data(matrix_y,xcol,0);
        }
        data=(matrix_get_data(matrix_pb,row,0)-sum);
        matrix_set_data(&matrix_y,row,0,data);
    }

    for(row=row_size-1;row>=0;row--){
        sum=0;
        for(xcol=row;xcol<row_size;xcol++){
            sum+=matrix_get_data(upper,row,xcol)*matrix_get_data(matrix_x,xcol,0);
        }
        data=(matrix_get_data(matrix_y,row,0)-sum)/matrix_get_data(upper,row,row);
        matrix_set_data(&matrix_x,row,0,data);
    }

    return matrix_x;
  */






  //double eps = 1.0e-15;
  //check that B has only one column...
  //if (B->columns() > 1) return NULL;
  //check that A is square...
  if ( A->rows() != A->columns() )
  {
    return NULL;
  }
  int n = A->rows();
  //create multiplier matrix
  QgsMatrix M = QgsMatrix( n, n );
  //create augmented matrix
  QgsMatrix Au = QgsMatrix( n, n + 1 );
  for ( int i = 0; i < n; i++ )
  {
    for ( int j = 0; j < n; j++ )
    {
      Au( i, j ) = ( *A )( i, j );
    }
    Au( i, n ) = ( *B )( i, col );
  }

  //STEP 1
  int* nrow = new int[n];
  for ( int i = 0; i < n; i++ )
  {
    nrow[i] = i;
  }

  //STEP 2
  //printf( "2\n" );
  int p;
  int maxj;
  for ( int i = 0; i < n - 1; i++ )
  {
    //STEP 3
    bool flag = false;
    //printf( "3\n" );
    //find max
    maxj = i;
    for ( int j = i; j < n; j++ )
    {
      if ( fabs( Au( nrow[j], i ) ) > fabs( Au( nrow[maxj], i ) ) ) maxj = j;
    }
    for ( p = i; p < n && !flag; p++ )
    {
      if ( fabs( Au( nrow[p], i ) ) == fabs( Au( nrow[maxj], i ) ) ) flag = true;
    }
    p--;

    //STEP 4
    //printf( "4\n" );
    if ( FLT_ZERO( Au( nrow[p], i ) ) )
    {
      delete [] nrow;
      return NULL;
    }

    //STEP 5
    //printf( "5\n" );
    if ( nrow[i] != nrow[p] )
    {
      int ncopy = nrow[i];
      nrow[i] = nrow[p];
      nrow[p] = ncopy;
    }

    //STEP 6
    //printf( "6\n" );
    for ( int j = i + 1; j < n; j++ )
    {
      //STEP 7
      //printf( "7\n" );
      M( nrow[j], i ) = Au( nrow[j], i ) / Au( nrow[i], i );
      //STEP 8
      //printf( "8\n" );
      for ( int k = 0; k < n + 1; k++ )
      {
        Au( nrow[j], k ) = Au( nrow[j], k ) - ( M( nrow[j], i ) * Au( nrow[i], k ) );
      }
    }
  }

  //STEP 9
  //printf( "7\n" );
  if ( FLT_ZERO( Au( nrow[n-1], n - 1 ) ) )
  {
    delete nrow;
    return NULL;
  }

  //STEP 10
  //printf( "8\n" );
  ( *X )( n - 1, col ) = Au( nrow[n-1], n ) / Au( nrow[n-1], n - 1 );

  //STEP 11
  //printf( "9\n" );
  for ( int i = n - 2; i >= 0; i-- )
  {
    double subtract = 0;
    for ( int j = i + 1; j < n; j++ )
    {
      subtract += Au( nrow[i], j ) * ( *X )( j, col );
    }
    ( *X )( i, col ) = ( Au( nrow[i], n ) - subtract ) / Au( nrow[i], i );
  }
  delete []nrow;
  return X;
}

QgsMatrix* QgsMatrix::solveLowerTriangular( QgsMatrix* X, const QgsMatrix* A, const QgsMatrix* B )
{
  //check that A is square
  if ( A->rows() != A->columns() ) return NULL;
  //check lower triangularity
  for ( int t = 0; t < A->rows(); t++ )
  {
    if (( *A )( t, t ) == 0.0 ) return NULL;
    for ( int s = t + 1; s < A->columns(); s++ )
    {
      if (( *A )( t, s ) != 0.0 ) return NULL;
    }
  }
  //check that A->rows == X->rows() == B->rows()
  if ( !( A->rows() == X->rows() && A->rows() == B->rows() ) ) return NULL;
  //check that X->columns() == B->columns()
  if ( X->columns() != B->columns() ) return NULL;

  int rowSize = A->rows();
  int colSize = B->columns();
  for ( int i = 0; i < colSize; i++ )
  {
    for ( int j = 0; j < rowSize; j++ )
    {
      double subtract = 0;
      for ( int k = 0; k < j; k++ )
      {
        subtract += ( *A )( j, k ) * ( *X )( k, i );
      }
      ( *X )( j, i ) = (( *B )( j, i ) - subtract ) / ( *A )( j, j );
    }
  }
  return X;
}

QgsMatrix* QgsMatrix::solveUpperTriangular( QgsMatrix* X, const QgsMatrix* A, const QgsMatrix* B )
{
  //check that A is square
  if ( A->rows() != A->columns() ) return NULL;
  //check upper triangularity
  for ( int t = 0; t < A->rows(); t++ )
  {
    if (( *A )( t, t ) == 0.0 ) return NULL;
    for ( int s = t - 1; s >= 0; s-- )
    {
      if (( *A )( t, s ) != 0.0 ) return NULL;
    }
  }
  //check that A->rows == X->rows() == B->rows()
  if ( !( A->rows() == X->rows() && A->rows() == B->rows() ) ) return NULL;
  //check that X->columns() == B->columns()
  if ( X->columns() != B->columns() ) return NULL;

  int rowSize = A->rows();
  int colSize = B->columns();
  for ( int i = 0; i < colSize; i++ )
  {
    for ( int j = rowSize - 1; j >= 0; j-- )
    {
      double subtract = 0;
      for ( int k = j + 1; k < rowSize; k++ )
      {
        subtract += ( *A )( j, k ) * ( *X )( k, i );
      }
      ( *X )( j, i ) = (( *B )( j, i ) - subtract ) / ( *A )( j, j );
    }
  }
	return X;
}


//**************************END OF STATIC METHODS*************************

//************************************************************************
// OPERATORS
//************************************************************************

QgsMatrix QgsMatrix::operator*( const QgsMatrix& other ) const
{
  QgsMatrix m( mRows, other.mCols, false );
  if ( mCols == other.mRows )
  {
    QgsMatrix::multiply( &m, this, &other );
  }
  return m;
}

QgsMatrix QgsMatrix::operator+( const QgsMatrix& other ) const
{
  QgsMatrix m( *this );
  m += other;
  return m;
}

QgsMatrix QgsMatrix::operator-( const QgsMatrix& other ) const
{
  QgsMatrix m( *this );
  m -= other;
  return m;
}

QgsMatrix QgsMatrix::operator*( const double& value ) const
{
  QgsMatrix m( *this );
  m *= value;
  return m;
}

QgsMatrix QgsMatrix::operator/( const double& value ) const
{
  QgsMatrix m( *this );
  m /= value;
  return m;
}

QgsMatrix& QgsMatrix::operator*=( const QgsMatrix & other )
{
  if ( columns() == other.columns()
       && rows() == other.rows() )
  {
    QgsMatrix::multiply( this, this, &other );
  }
  return *this;
}
QgsMatrix& QgsMatrix::operator+=( const QgsMatrix & other )
{
  QgsMatrix::add( this, this, &other );
  return ( *this );
}
QgsMatrix& QgsMatrix::operator-=( const QgsMatrix & other )
{
  QgsMatrix::subtract( this, this, &other );
  return ( *this );
}
QgsMatrix& QgsMatrix::operator*=( const double & value )
{
  QgsMatrix::multiply( this, this, value );
  return ( *this );
}
QgsMatrix& QgsMatrix::operator/=( const double & value )
{
  return *QgsMatrix::divide( this, this, value );
}

bool QgsMatrix::operator==( const QgsMatrix& other )const
{
  if ( rows() != other.rows() || columns() != other.columns() )
  {
    return false;
  }
  int elements = rows() * columns();
  for ( int i = 0; i < elements; ++i )
  {
    // printf("%f ?= %f\n",mData[i], other.mData[i]);
    if ( !FLT_EQUALS( mData[i], other.mData[i] ) )
      return false;
  }
  return true;
}