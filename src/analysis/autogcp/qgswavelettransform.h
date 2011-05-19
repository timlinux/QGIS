/***************************************************************************
qgswavelettransform.h - Extracts salient points from a raster image using the
 Haar wavelet transform
--------------------------------------
Date : 14-September-2010
Author : James Meyer
Copyright : (C) 2010 by C.J. Meyer
Email : jamesmeyerx@gmail.com

***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgswavelettransform.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSWAVELETTRANSFORM_H
#define QGSWAVELETTRANSFORM_H
#include <gdal.h>
#include <gdal_priv.h>
#include <QLinkedList>
#include <QList>
#include "qgssalientpoint.h"
#include "qgsgcpset.h"
#include "qgsrasterdataset.h"
#define DEFAULT_LEVELS 5
#define SALIENCY_LIMIT 1
#define SAL_LIMIT 175
#define DEFAULT_BLACK_LIMIT 0.005
#define DEFAULT_RASTERBAND 1
#define DEFAULT_MAX_FEATURES 3000
#define DEFAULT_THRESHOLD 100.0
typedef void( *QgsProgressFunction )( double, void* );
/*!\ingroup analysis
 *
 * The class uses the Haar Wavelet transform to detect salient features on an image.
 * The Haar wavelet transform decomposes the image into different resolution representations of the image by calculating averages.
*/
class ANALYSIS_EXPORT QgsWaveletTransform// : public QgsFeatureExtractor
{
  public:
    typedef QList<QgsSalientPoint> PointsList;

    /*! \brief QgsWaveletTransform Constructor.
    Initializes the wavelet extractor with the given image and default feature size.
    */
    QgsWaveletTransform( QgsRasterDataset* image );

    /*! \brief QgsWaveletTransform destructor
    This does not free the QgsRasterDataset object or any point data retrieved.
    */
    ~QgsWaveletTransform();

    /*! \brief Performs the wavelet transform to collect salient points on the image.
     * \return True on success or false on failure.
     */
    bool execute();

    /*! \brief Extracts the detected points in a distributed manner
     *
     * The level of distribution depends on the columns and rows set using setDistribution().
     * The method tries to extract equal amount of points from each block inside the image,
     * returning only the points with highest saliency in these blocks
     * \param quantity The amount of points to extract in total.
     * \return A pointer to the heap-allocated list of points. Caller is responsible for deleting this list.
     */
    PointsList* extractDistributed( int quantity ) const;

    /*! \brief Extracts the best points from a specific block inside the image.
     *
     * The amount of blocks depends on the values given to setDistribution().
     * The returned list is ordered by saliency (level of image variance).
     * \param quantity The amount of points to extract from the block
     * \param column The column index of the block
     * \param row The row index of the block
     * \param offset The offset into the list of points to start extracting from
     * \return A pointer to the heap-allocated list of points. Caller is repsonsible for deleting this list.
     */
    PointsList* extract( int quantity, int column, int row, int offset = 0 ) const;

    /*! \brief Gets a pointer to the list of points of a specific block */
    const PointsList* constList( unsigned int column, unsigned int row ) const;

    /*! \brief Checks wether this extractor initialized correctly and is ready for extraction.
     *
     * If this method returns false, a likely reason is that the underlying image is invalid.
    */
    bool initialized();

    /*! \brief Sets the raster band to use for feature detection
     *
     * @note Band numbers are indexed from <b>1</b>.
     * \param band The 1-indexed band number to use in the wavelet transform
    */
    void setRasterBand( int band );

    /*! \brief Gets the raster band that will be used for feature extraction
     */
    int rasterBand();

    /*! \brief Sets the size of the features to find.
     *
     * The pixel and line sizes given here are always rounded up to the next power of 2 (32x32, 64x64)
     * The size of the features that should be detected affects the amount of features that can be detected.
     */
    void setFeatureSize( int xSize, int ySize );

    /*!\brief Limits the amount of points to detect per block
     *
     * A limit ensures that the list of points to maintain in the list during
     * extraction does not grow too large. This does applies to each individual
     * internal list and not the process as a whole.
     * Setting a limit will increase the performance when the features are stored,
     * ordered by saliency (the default), as insertions can take very long for long lists.
     *
     * \sa setOrdering()
     */
    void setMaxFeatures( int value ) {mMaxGcps = value;}

    /*!\brief Sets whether detected points are stored ordered by saliency value
     *
     * The default is true. For large images that may produce several thousands
     * of salient points, it is advised to limit the amount of features that can be detected per block.
     *
     * \sa setMaxFeatures()
     */
    void setOrdering( bool value ) {mOrder = value;}

    /*! \brief Sets the amount of grid blocks to use for the optimal distribution of extracted points.
     *
     * Defaults to just 1 (no distribution)
     */
    void setDistribution( int columns, int rows ) { mCols = columns; mRows = rows;}

    /*! \brief Sets the optimal internal distribution based on the required amount of control points
     *
     */
    void suggestDistribution( int nNumPoints );
    /*! \brief Gets the amount of columns and rows governing the distribution of points
     *
     * \sa setDistribution()
     */
    void distribution( int& columns, int& rows ) { columns = mCols; rows = mRows;}

    /*! \brief Sets the fraction of black that will cause a point to ignored
     *
     * Many images are rotated and contain black padding around the edges.
     * Because the edges between actual image data and this black padding result
     * in very high image variance, they are incorrecly regarded as salient points.
     * Setting a limit of the amount of black pixels reduces this probability.
     *
     * The value must be between 0 (no black allowed) and 1.0 (May be all black).
     * The default value is 0.005.
     *
     */
    void setMaxBlackCoverage( double value ) {mBlackLimit = value;}

    /*! \brief Gets the fraction of black pixels that will cause a point to be disregarded */
    double maxBlackCoverage()const {return mBlackLimit;}

    void setProgressFunction( QgsProgressFunction pfnProgress );
    void setProgressArgument( void* pProgressArg );
  private:
    bool initialize();
    static void addPoint( PointsList* list, QgsSalientPoint item, bool order = true, int limit = -1 );
    PointsList* featureList( int pixel, int line );

    /*!\brief Takes a single chip and does the wavelet transform
     * The Output point has coordinates relative to the chip's position.
     * For performance reasons this method requires all the inputs below,
     * to minimize calculations, as it is called for every feature-sized block of data.
     * \param buffer The block of raster data to transform
     * \param cache An array of pointers at least <i>levels</i> blocks of allocated memory for operations.
     * \param levels The number of decomposition levels.
     * \param out The Salient point to receive the result.
     */
    bool transform( double* buffer, double** cache, int size, int levels, QgsSalientPoint* out );

    /*! \brief Finds the value that is furthest from the specified average
    Helper method called in reconstruct()
    */
    char findMaxDev( double* values, double avg );
    //void* readData() {};//Not used yet
  private:
    QgsRasterDataset* mImage;
    int mRasterXSize, mRasterYSize;
    int mPaddedSize;
    int mLevels, mChipSize;
    int mRasterBand;
    bool mInitialized, mExecuted;
    bool mOrder;
    int mMaxGcps;
    double mBlackLimit;
    double mThreshold, mLimit;
    int mCols, mRows;
    PointsList* mGridLists;
    QgsProgressFunction mProgressFunction;
    void* mProgressArg;

};
//Get the highest deviating value of the four components of one coefficient
inline char QgsWaveletTransform::findMaxDev( double* values, double avg )
{
  char max = 0;
  double total = 0;
  for ( char i = 0; i < 4; ++i )
  {
    total += values[i];
    if ( abs( values[i] - avg ) > abs( values[max] - avg ) )
    {
      max = i;
    }
    // std::cout << values[i] << ", ";
  }
// std::cout <<std::endl;
  //printf("Average compare. Total: %6.3f Read: %6.3f, calc: %6.3f \n",total, avg, total / 4.0f);

  return max;
}
#endif
