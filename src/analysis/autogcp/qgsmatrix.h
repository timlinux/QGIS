/***************************************************************************
qgsmatrix.h - A two dimensional m x n matrix
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
//--Insert SVN ID!
#ifndef QGSMATRIX_H
#define QGSMATRIX_H

#include <float.h>
#ifndef FLT_EQUALS
#define FLT_EQUALS(x,y) \
  (fabs(x-y)<FLT_EPSILON)
#endif
#ifndef FLT_ZERO
#define FLT_ZERO(x) \
  (fabs(x)<FLT_EPSILON)
#endif
/*! \ingroup analysis*/
/*! \brief A two dimensional m x n matrix
 * The matrix is represented by 2-dimensional m x n array of double values.
 */
class ANALYSIS_EXPORT QgsMatrix
{
  public:
    /*!\brief Constructs a matrix with the specified dimensions
     * \param rows The number of rows
     * \param cols The number of columns
     * \param initialize If set to true (default) all elements in this matrix will be initialized to 0.0
     */
    QgsMatrix( int rows, int cols, bool initialize = true );
    /*! \brief Copy Constructor
     * Creates a matrix with the same dimensions as <i>other</i> and copies all the values to the new matrix
     */
    QgsMatrix( const QgsMatrix& other );
    /*! \brief Assignment operator
     * Assigns all the values from <i>other</i> to this matrix, is their dimensions are the same.
     */
    const QgsMatrix& operator=( const QgsMatrix& other );
    /*! \brief Matrix destructor*/
    ~QgsMatrix();
    /*! \brief Resizes the matrix to the specified dimension
     * Copies old elements from the matrix to the dimension possible. If the matrix is shrunk some values will be discarded.
     * \param rows The new number of rows
     * \param cols The new number of columns
     * \param initialize If set to true and the new size is larger than the previous, then any new values will be initialized to 0.0
     */
    void resize( int rows, int cols, bool initialize = true );
    /*! \brief Creates a new matrix that is the transpose of this matrix
     */
    QgsMatrix transpose() const;
    /*! \brief Creates a new matrix that is the inverse of this matrix
     */
    QgsMatrix inverse() const;
    /*! \brief Checks whether this matrix is a square identity matrix
     */
    bool isIdentity() const;
    //***********Static Operations*********************//
    // These pass-by-pointer for faster calling, but require existing in/out matrices
    // The member functions make use of these methods
    // All these functions return a pointer to the out parameter, or NULL on failure
    /*!\brief Creates an identity matrix and stores it in <i>out</i>
     * The matrix pointed-to by <i>out</i> must already have been initialized with equal rows and columns
     * \return Returns a pointer to out on success, or NULL if an invalid matrix was given as input parameter
     */
    static QgsMatrix* identity( QgsMatrix* out );
    /*! \brief Performs matrix multiplication between <i>left</i> and <i>right</i> and stores the result in <i>out</i>
     * The dimensions of <i>left</i>, <i>right</i> and <i>out</i> must all be valid for this function to succeed
     * \param out A pointer to the destination matrix for this operation
     * \param left A pointer to the left-hand matrix of the multiplication
     * \param right A pointer to the right-hand matrix of the multiplication
     * \return Returns a pointer to <i>out</i> or NULL on failure.
     */
    static QgsMatrix* multiply( QgsMatrix* out, const QgsMatrix* left, const QgsMatrix* right );
    /*! \brief Performs matrix addition between <i>left</i> and <i>right</i> and stores the result in <i>out</i>
     * The two input matrices and the output matrix must all have the same dimension
     * \param out A pointer to the destination matrix for this operation
     * \param left A pointer to the left-hand matrix for this operation
     * \param right A pointer to the right-hand matrix for this operation
     * \return Returns a pointer to <i>out</i> or NULL on failure.
     */
    static QgsMatrix* add( QgsMatrix* out, const QgsMatrix* left, const QgsMatrix* right );
    /*! \brief Performs matrix subtraction between <i>left</i> and <i>right</i> and stores the result in <i>out</i>
     * The two input matrices and the output matrix must all have the same dimension
     * \param out A pointer to the destination matrix for this operation
     * \param left A pointer to the left-hand matrix for this operation
     * \param right A pointer to the right-hand matrix for this operation
     * \return Returns a pointer to <i>out</i> or NULL on failure.
     */
    static QgsMatrix* subtract( QgsMatrix* out, const QgsMatrix* left, const QgsMatrix* right );
    /*! \brief Performs scalar multiplication of <i>in</i> with <i>value</i> and stores the result in <i>out</i>
     * The dimension of the matrix pointed to by <i>out</i> must be the same as the input matrix
     * \param out A pointer to the destination matrix for this operation
     * \param in A pointer to the input matrix
     * \param value The scalar factor
     * \return Returns a pointer to <i>out</i> or NULL on failure.
     */
    static QgsMatrix* multiply( QgsMatrix* out, const QgsMatrix* in, double value );
    /*! \brief Performs scalar division of <i>in</i> with <i>value</i> and stores the result in <i>out</i>
     * The dimension of the matrix pointed to by <i>out</i> must be the same as the input matrix
     * \param out A pointer to the destination matrix for this operation
     * \param in A pointer to the input matrix
     * \param value The scalar divisor
     * \return Returns a pointer to <i>out</i> or NULL on failure.
     */
    static QgsMatrix* divide( QgsMatrix* out, const QgsMatrix* in, double value );
    /*! \brief Calculates the transpose of the input matrix and stores the result in <i>out</i>
     * The dimension of the matrix pointed to by <i>out</i> must be the opposite of the input matrix
     * \param out A pointer to the destination matrix for this operation
     * \param in A pointer to the input matrix
     * \return Returns a pointer to <i>out</i> or NULL on failure.
     */
    static QgsMatrix* transpose( QgsMatrix* out, const QgsMatrix* in );
    /*! \brief Determines the inverse of the matrix <i>in</i>
    The inverted matrix is stored in the matrix pointed to by <i>out</i>
    \return Returns a pointer to the out matrix
    */
    static QgsMatrix* invert( QgsMatrix* out, const QgsMatrix* in );
    /*! \brief  Solves for X in the equation AX = B
    \return Returns a pointer to X or NULL on failure
    */
    static QgsMatrix* solve( QgsMatrix* X, const QgsMatrix* A, const QgsMatrix* B );

    static QgsMatrix* solveVector( QgsMatrix* X, const QgsMatrix* A, const QgsMatrix* B, int col = 0 );

    /*! \brief Solves X in equation AX = B for a lower triangular matrix A and stores the result in <i>X</i>
        * The dimension of the matrix pointed to by <i>X</i> must be the same as the input matrix <i>B</i>
     * The matrix pointed to by <i>A</i> must be lower triangular
        * \param out A pointer to the destination matrix for this operation
        * \param in A pointer to the lower triangular input matrix
        * \param value A pointer to the input matrix
        * \return Returns a pointer to <i>out</i> or NULL on failure.
        */
    static QgsMatrix* solveLowerTriangular( QgsMatrix* X, const QgsMatrix* A, const QgsMatrix* B );

    /*! \brief Solves X in equation AX = B for an upper triangular matrix A and stores the result in <i>X</i>
        * The dimension of the matrix pointed to by <i>X</i> must be the same as the input matrix <i>B</i>
     * The matrix pointed to by <i>A</i> must be upper triangular
        * \param out A pointer to the destination matrix for this operation
        * \param in A pointer to the upper triangular input matrix
        * \param value A pointer to the input matrix
        * \return Returns a pointer to <i>out</i> or NULL on failure.
        */
    static QgsMatrix* solveUpperTriangular( QgsMatrix* X, const QgsMatrix* A, const QgsMatrix* B );

    static int Doolittle( QgsMatrix* Am, int pivot[], int n );

    //*************Operators*******************************//
    /*! \brief Returns the matrix that is the result of multiplying this matrix with <i>other</i>
     */
    QgsMatrix operator*( const QgsMatrix& other ) const;
    /*! \brief Returns the matrix that is the result of adding this matrix to <i>other</i>
     */
    QgsMatrix operator+( const QgsMatrix& other ) const;
    /*! \brief Returns the matrix that is the result subtracting <i>other</i> from this matrix
     */
    QgsMatrix operator-( const QgsMatrix& other ) const;
    /*! \brief Returns the matrix that is the result of multiplying this matrix with a scalar value
     */
    QgsMatrix operator*( const double& value ) const;
    /*! \brief Returns the matrix that is the result of dividing this matrix by a scalar value
     */
    QgsMatrix operator/( const double& value ) const;

    /*! \brief Multiply this matrix with <i>other</i> and store the result back into this matrix
     * This function only succeeds if both matrices are square and of the same dimension
     */
    QgsMatrix& operator*=( const QgsMatrix& other );
    /*! \brief Adds this matrix to <i>other</i> and store the result back into this matrix
     * This function only succeeds if both matrices are square and of the same dimension
     */
    QgsMatrix& operator+=( const QgsMatrix& other );
    /*! \brief Subtracts <i>other</i> from this matrix and store the result back into this matrix
     * This function only succeeds if both matrices are square and of the same dimension
     */
    QgsMatrix& operator-=( const QgsMatrix& other );
    /*! \brief Multiply this matrix with a scalar value and store the result back into this matrix
     */
    QgsMatrix& operator*=( const double& value );
    /*! \brief Divide this matrix by a scalar value and store the result back into this matrix
     * This function only succeeds if both matrices are square and of the same dimension
     */
    QgsMatrix& operator/=( const double& value );
    /*! \brief Gets the value of the element at the given row and column index
     */
    double operator()( int row, int col ) const {return mData[mCols*row + col];}
    /*! \brief Gets a reference to the element at the given row and column index
     */
    double& operator()( int row, int col ) {return mData[mCols*row + col];}

    /*! \brief Tests whether the matrix is equal to the other
     * This compares the dimensions and all the elements in both matrices.
     * Elements are tested using floating point equality to compensate for rounding errors.
     */
    bool operator==( const QgsMatrix& other )const;


    //*************GETTERS & SETTERS***********************//
    /*! \brief Gets the number of rows in this matrix
     */
    int rows()const {return mRows;}
    /*! \brief Gets the number of columns in this matrix
     */
    int columns()const {return mCols;}
    /*! \brief Gets the value of the element at the given row and column index
     */
    double element( int row, int col ) const
    {
      if ( validIndex( row, col ) ) {return ( *this )( row, col );}
      else {return 0.0;}
    };
    /*! \brief Sets the value of the element at the given row and column index
     */
    void setElement( int row, int col, double value ) {if ( validIndex( row, col ) ) {( *this )( row, col ) = value; } }
    /*! \brief Tests whether the given row and column index is valid for this matrix
     */
    bool validIndex( int row, int col ) const {return ( mData && row >= 0 && row < mRows && col >= 0 && col < mCols );}
  protected:

    /* typedef struct{
       int row_size, // size of row
       col_size; // size of col
       float *data;  // data pointer
     }matrix_t;*/

  private:
    int mRows, mCols;
    double* mData;

};

#endif
