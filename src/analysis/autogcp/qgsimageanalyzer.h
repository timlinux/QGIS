/***************************************************************************
qgsimageanalyzer.cpp - The ImageAnalyzer class is responsible for the physical
 analysis and processing of image data. This included detecting and extracting
 GCPs and GCP chips from images, as well as searching for these reference
 GCP chips on a raw image.
--------------------------------------
Date : 07-May-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com

***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsimageanalyzer.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSIMAGEANALYZER_H
#define QGSIMAGEANALYZER_H
#include "qgsgcpset.h"
#include "qgsabstractoperation.h"
#include "qgsrasterdataset.h"
#include "qgssalientpoint.h"
#include "qgswavelettransform.h"
#include "qgsabstractoperation.h"
#include <gdal.h>
#include <gdal_priv.h>
#define CHIPTYPE GDT_Byte
#define DEFAULT_CHIP_SIZE 32
#define DEFAULT_CHIP_BANDS -1
#define UNKNOWN_TYPE GDT_Unknown
#define WAV_DISTRIB 10
#define SEARCH_WINDOW_MULTIPLIER 6.0
#define SEARCH_FEATURE_MULTIPLIER 0.5
#define DEFAULT_BAND 1
#define MATCH_ASSUMED 0.98
/*! \ingroup analysis
 *
 * The ImageAnalyzer class is responsible for the physical
 * analysis and processing of image data. This included detecting and extracting
 * GCPs and GCP chips from images, as well as searching for these reference
 * GCP chips on a raw image.
 */
class ANALYSIS_EXPORT QgsImageAnalyzer : public QgsAbstractOperation
{

    friend class QgsGrid;
  public:
    /*! \brief QgsImageAnalyzer Constructor
    Constructs an analyzer for the supplied image, ready to perform processing on this image.
    */
    QgsImageAnalyzer( QgsRasterDataset* image = 0 );
    /*! \brief Extracts ground control points from the assigned image.
    This method allocates a new QgsGcpSet and transfers ownership to the caller.
    The GCP's contained in this set will have geographic reference coordinates and a usable Chip for matching.
    If this method fails it returns NULL
    */
    QgsGcpSet* extractGcps( int amount );
    /*! \brief Matches the supplied GCP set to the image dataset, using the GCP chips.
    If the method was successful the supplied GCP set will have it's raw coordinates updated.
    \param A pointer to the GCP set to use and update.
    \return A pointer to the supplied GCP set, with updated coordinates, or NULL on failure
    */
    QgsGcpSet* matchGcps( QgsGcpSet* gcpSet );
    /*! \brief Brute force approach to matching the given GCP set.
     */
    QgsGcpSet* bruteMatchGcps( QgsGcpSet* gcpSet );
    /*! \brief QgsImageAnalyzer Destructor
    */
    virtual ~QgsImageAnalyzer();

    /*! \brief Extracts an area of this image analyzer's dataset into a new dataset.
    The size of the area to be extracted must be specified, as well as the point on the source image to start copying from.
    The data is read into the destination dataset or, if NULL is specified a new in-memory raster is created for the chip.
    The datatype of the extracted chip will be the same
    \param sourcePoint The x, y pixel coordinates on the source image to start copying from.
    \param xPixels The width in pixels of the area to extract.
    \param yLines The height in lines of the area to extract.
    \param bands The number of bands to extract. If set to -1 the method will extract as many as the source dataset has.
    \param type The datatype of the destination chip. If unknown it defaults to source datatype.
    \param dest The destination dataset.
    */
    QgsRasterDataset* extractChip( QgsPoint& sourcePoint, int xPixels, int yLines, int bands = -1, GDALDataType destType = UNKNOWN_TYPE, QgsRasterDataset* dest = NULL );
    QgsRasterDataset* extractArea( int offsetX, int offsetY, int width, int height, int bands = -1, GDALDataType destType = UNKNOWN_TYPE, QgsRasterDataset* dest = NULL );
    //*****************GETTERS & SETTERS****************************************************
    /*! \brief Sets the image dataset of this analyzer.
    This can be used to change the image between operations, to enable reuse of the same QgsImageAnalyzer object.
    */
    void setImage( QgsRasterDataset* image ) {mImageData = image;}
    /*! \brief Gets a pointer to the raster begin used for operation*/
    QgsRasterDataset* image() const {return mImageData;}
    /*! \brief Sets the band number to use for all transformations*/
    void setTransformBand( int bandNumber );
    /*! \brief Gets the current band number for operations*/
    int transformBand() const {return mBandNumber;}
    /*! \brief Sets the feature/chip size to use when extracting salient points from an image*/
    void setChipSize( int xSize, int ySize ) {mChipSize.width = xSize; mChipSize.height = ySize;}
    /*! \brief Gets the current X-Size of GCP chips*/
    int chipXSize() const {return mChipSize.width;}
    /*! \brief Gets the current Y-size of GCP chips*/
    int chipYSize() const {return mChipSize.height;}
    /*! \brief Sets the number of bands to copy into GCP chips*/
    void setChipBandCount( int bands ) {mChipBandCount = bands;}
    /*! \brief Gets the number of bands copied into GCP chips*/
    int chipBandCount() const {return mChipBandCount;}
    /*! \brief Gets the datatype used when creating GCP chips*/
    GDALDataType chipDataType()const {return mChipType;}
    /*! \brief Sets the datatype used when creating GCP chips*/
    void setChipDataType( GDALDataType type ) {mChipType = type;}
    /*! \brief Gets the progress of the currently executing operation*/
    double progress()const;
    /*! \brief Sets the current progress*/
    void setProgress( double value );
    /*! \brief Gets the currently used correlation threshold*/
    double correlationThreshold()const;
    /*! \brief Sets the correlation threshold*/
    void setCorrelationThreshold( double value );
    //********************************************************************************************
  private:
    //************PRIVATE TYPES;
    typedef struct
    {
      int width;
      int height;
    } Dimension;

    typedef struct GridBlock
    {
      int xBegin, yBegin, xEnd, yEnd;
      QList<QgsPoint> Points;
    } GridBlock;

    //**************PRIVATE CLASSES******************
    class QgsGrid
    {
      private:
        friend class QgsImageAnalyzer;
        QgsGrid( int saSize, QgsRasterDataset* image );
        ~QgsGrid();
        void fillGrid( const QgsWaveletTransform::PointsList* points );
        QList<QgsPoint>* gridList( QgsGcp* refGcp );
        GridBlock* blocks;
        QgsRasterDataset* mImageData;
        int rows, cols, blockSize;
    };
    //**************PRIVATE METHODS*******************
    static void CPL_STDCALL setWaveletProgress( double progress, void* progressArg );
  private:
    //**************PRIVATE MEMBERS******************
    QgsRasterDataset* mImageData;
    double mCorrelationThreshold;
    Dimension mChipSize;
    int mChipBandCount;
    int mBandNumber;
    GDALDataType mChipType;
    double mSearchFeatureRatio;
    double mProgress;
};

static const double WAVELET_PART = 0.7;

#endif // QGSIMAGEANALYZER_H
