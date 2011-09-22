#ifndef QGSIMAGEALIGNER_H
#define QGSIMAGEALIGNER_H

#include "qgsprogressmonitor.h"

#include <QString>
#include <QList>
#include <gdal_priv.h>

#if 0
/*
This class captures the complextities of alligning images/bands typical usage would be:
  -> initiate the class with the images and the bands of interest as well as the block size [for more info look at the init method]
  -> call the scan method to generate a disperity map. 
  -> (the disparity map can be obained by the Estimate method [currently using bilinear interpolation)
  -> then the corrected image is obtained from ApplyChanges
  -> (the MeasureDiff method can be used to compare the corrected image to the base image)
*/

class ANALYSIS_EXPORT QgsImageAligner
{
  public:
    
    /*
    Default constructor
    */
    //QgsImageAligner();
    
    /*
    Constructor
    image1: The base/refrence image
    image2: The translated/deformed/unalligned image
    band1: The band to be used of image1
    band2: The band to be used of image2
    blockSize: The block size to be used 
    */
    QgsImageAligner(QString inputPath, QString translatePath, int referenceBand = 1, int unreferencedBand = 1, int blockSize = 201);
    
    /*
    Destructor
    */
    ~QgsImageAligner();
    

  protected:
    /*
    This method scans through the image at given block sizes
    */
    //void scan();

    /*
    This function returns an estimated Disparity matrix based on the current Real_Disp (Real Dispariry Matrix)
    returns an interpolated disparity map
    */
    //void estimate();
    
    /*
    Eliminate bad GCPs
    fixed: the values that are fixed from previous levels
    */
    //void eliminate(double sigmaWeight = 2.0);
    
    /*
    This method applies the disparity map to the imag_trans 
    returns a corrected transformation of imag_trans
    */
    //uint *applyUInt();
    //int *applyInt();
    //double *applyFloat();
    //void alignImageBand(GDALDataset *disparityMapX, GDALDataset *disparityMapY, GDALDataset *outputData, int nBand);      
    
    //int width(){return mWidth;}
    //int height(){return mHeight;}
    //double* disparityReal(){return mDisparityReal;}
    //double* disparityImag(){return mDisparityImag;}
    //QgsComplex **realDisparity(){return mRealDisparity;}
    //QgsComplex **gcps(){return mReal;}
        
    /*
    Open the images
    */
    void open();
    
    /*
    This function implements the cubic convolution
    inputs block: The block is of size 4*4
    point: Is the shift x = point.real y = point.imag !!
    returns point: Is the cubic convolution depending on the shift in point
    */
    //int cubicConvolution(double *array, QgsComplex point);
    
    //QgsComplex findPoint(double *point, QgsComplex estimate = 0);
    //uint region(GDALRasterBand* rasterBand, QMutex *mutex, double point[], int dimension, QgsRegion &region);
    
    //bool isBad(double value, double deviation, double mean, double sigmaWeight = 2.0);
    
  private:
    //QString mInputPath;
    //QString mTranslatePath;
    //GDALDataset *mInputDataset;
    //GDALDataset *mTranslateDataset;
    //int mWidth;
    //int mHeight;
    //int mBits;
    //int mMax;
    //int mReferenceBand;
    //int mUnreferencedBand;
    //int mBlockSize;

    //QgsComplex **mRealDisparity;
    //double* mDisparityReal;
    //double* mDisparityImag;
    //QgsComplex **mReal;
};

#endif

#if 0
class ANALYSIS_EXPORT QgsPointDetectionThread : public QThread
{
  public:
    
    QgsPointDetectionThread(QMutex *mutex, GDALDataset *inputDataset, GDALDataset *translateDataset, int referenceBand, int unreferencedBand, int blockSize, int bits, int *max);
  
    /*
    This function finds a point within a region
    inputs
    Point: The point(on the base image) to found on the tran image
    original_estimate:	The original estimate we start with
    returns New_Estime:	this method returns a new estimate
    */
    void setPoint(double *point, QgsComplex estimate = 0);
    
    /*
     * This method reads a region from an image of size dim
     * 
     * inputs
     *  rasterBand : a handle to the gdal raster band of interest
     *  point : the central point of the region
     *  dim : the dimentions of the region    
     *  region : the requested region to be filled with data
     * returns 
     *  max : the maximum value of the pixels in the region
     */    
    uint QgsPointDetectionThread::region(GDALRasterBand *rasterBand, QMutex *mutex, double point[], int dimension, QgsRegion &region);
    
    void run();
    
    QgsComplex result(){return mResult;}
    int x(){return mPoint[0];}
    int y(){return mPoint[1];}
    
  private:
    
    QMutex *mMutex;
    int *mMax;
    int mBits;
    double *mPoint;
    QgsComplex mEstimate;
    QgsComplex mResult;
    int mBlockSize;
    int mReferenceBand;
    int mUnreferencedBand;
    GDALDataset *mInputDataset;
    GDALDataset *mTranslateDataset;
};
#endif

extern void performImageAlignment(
        QgsProgressMonitor &monitor,
        QString outputPath,
        QString disparityXPath,
        QString disparityYPath,
        QString disparityPath,
        QList<GDALRasterBand*> &mBands, 
        int nRefBand, 
        int nBlockSize);

#endif