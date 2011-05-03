/*! \ingroup analysis*/
/*! \brief A two dimensional m x n matrix
 * The matrix is represented by 2-dimensional m x n array of double values.
 */
class QgsMatrix
{
%TypeHeaderCode
#include <qgsmatrix.h>
%End
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

    //static int Doolittle( QgsMatrix* Am, int pivot[], int n );

    


    //*************GETTERS & SETTERS***********************//
    /*! \brief Gets the number of rows in this matrix
     */
    int rows()const;
    /*! \brief Gets the number of columns in this matrix
     */
    int columns()const;
    /*! \brief Gets the value of the element at the given row and column index
     */
    double element( int row, int col ) const;
    /*! \brief Sets the value of the element at the given row and column index
     */
    void setElement( int row, int col, double value );
    /*! \brief Tests whether the given row and column index is valid for this matrix
     */
    bool validIndex( int row, int col ) const;
};
