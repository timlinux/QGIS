/***************************************************************************
qgsrasterdataset.cpp - General purpose raster dataset and reader that provides
 access to the metadata of the image.
--------------------------------------
Date : 07-May-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsrasterdataset.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSRASTERDATASET_H
#define QGSRASTERDATASET_H

#ifndef FLT_EQUALS
//Fuzzy comparison of floating point values
#define FLT_EQUALS(x,y) \
  (fabs(x-y)<FLT_EPSILON)
#endif
#include "qgsautogcp.h"
#include "qgsabstractoperation.h"
#include <QString>
#include <gdal_priv.h>

/*! \ingroup analysis
 *
 * General purpose raster dataset and reader that provides access to the metadata of the image.
 */
class ANALYSIS_EXPORT QgsRasterDataset : public QgsAbstractOperation
{
  public:
    enum Access
    {
      ReadOnly = 0,
      Update = 1
    };

    /*! \brief QgsRasterDataset Constructor
     *
     * Constructs the dataset with the given path and initializes all internal data
     */
    QgsRasterDataset( QString path, Access access = ReadOnly, bool automaticallyOpen = true );
    /*! \brief QgsRasterDataset Constructor
     *
     * Constructs the dataset with the given GDAL dataset.
     */
    QgsRasterDataset( GDALDataset* dataset );
    /*! \brief QgsRasterDataset Destructor
     *
     * Destroys the QgsRasterDataset instance, releasing any owned resources.
    */
    bool open( QString path, Access access );

    /*! \brief Closes this raster, saving all changes to disk
     */
    void close();

    /*! \brief Returns whether this raster is currently open for read/write operations.
     */
    bool isOpen() const;

    /*! \brief Closes and reopens the raster file
     *
     * This operations saves any data written to the dataset to file and then
     * reopens the dataset for usage.
     * The file access flag used is the same as the one used to originally open
     * the file.
     * If the raster is closed when this method is called, it will be opened.
     *
     * \return true if the file was successfully reopened, or false if an error occurred and/or nothing was done.
     *
     * @note To check if the raster is still open after a call to this method failed, use isOpen()
     *
     */
    bool reopen();

    virtual ~QgsRasterDataset();
    /**\brief Returns a pointer to the internal GDALDataset instance or NULL if initialization failed*/
    GDALDataset* gdalDataset();
    /*!\brief Returns true if this object failed to initialize with the given file*/
    virtual bool failed() const;

    /*!
     * See failed() const;
     */
    bool operator!() const;
    /*!\brief Gets the width and height of the raster image in pixels
     *
     * Returns the width through the xSize reference parameter and the height through ySize.
     * If the image did not load or initialize correctly it does nothing.
     * Use failed() to check validity of the object.
     */
    void imageSize( int& xSize, int& ySize ) const;

    /*!\brief Gets the width of this image in pixels
     */
    int imageXSize() const;
    /*!\brief Gets the height of this image in pixels
     */
    int imageYSize() const;

    /*! \brief Gets the number of raster bands in the image.
     *
     * Returns the number of rasterbands in the image or 0 if it hasn't been initialized.
     */
    int rasterBands() const;

    /*! \brief Reads the value of the image at a specified point
     *
     * \param band The band number to read from
     * \param xPixel The column in pixels to read from
     * \param yLine The line in pixels to read from
     * \param error True if the read succeeds. False otherwise. Reasons for failure include an uninitialized dataset or out-of-bounds access attempt.
     * \return The result of the read operation
    */
    double readValue( int band, int xPixel, int yLine, bool* succeeded = NULL );


    /*! \brief Checks whether this object has initialized correctly
    */
    operator bool() const;
    /*!\brief Gets the file path of the image on disk represented by this dataset
     *
     */
    QString filePath() const;

    /*! \brief
     */
    bool isVirtual() const;

    /*!\brief Returns the Geographic coordinates of the top-left corner of the image
     */
    void geoTopLeft( double& X, double& Y ) const;
    /*!\brief Returns the Geographic coordinates of the bottom-right corner of the image
     */
    void geoBottomRight( double& X, double& Y ) const;

    static const int TM_PIXELGEO = 0; //FROM PIXEL to GEO-COORDS
    static const int TM_GEOPIXEL = 1;//FROM GEO to PIXEL COORDS
    /*! \brief Transforms coordinates between image and ground space using affine transformation coefficients
     *
     * \param pixelX The x-coordinate in pixels
     * \param pixelY The y-coordinate in pixels
     * \param geoX The ground x-coordinate
     * \param geoY The ground y-coordinate
     * \param mode Flag indicating whether the ground coordinates should be calculated from image coordinates or vice versa.
     */
    bool transformCoordinate( int& pixelX, int& pixelY, double& geoX, double& geoY, const int mode = TM_PIXELGEO ) const;

    /*!\brief Gets the data type of this dataset
     *
     * \return Returns the GDALDataType of this image dataset
     */
    GDALDataType rasterDataType() const;
    /*!\brief Checks whether this image has georeferencing information set.
     *
     * \return Returns true if the image has 6 georeferencing coefficients set and false otherwise
     */
    bool georeferenced() const;
    /*!\brief Sets the Georeference coefficients of this image
     *
     * \param topLeftX The x-coordinate of the upper left corner of the image
     * \param pixelWidthX The x-component of the pixel width
     * \param pixelWidthY The y-component of the pixel width (in the case of rotated images)
     * \param topLeftY The y-coordinate of the upper left corner of the image
     * \param pixelHeightX The x-component of the pixel height (in the case of rotated images)
     * \param pixelHeightY The y-component of the pixel height
     */
    void setGeoCoefficients( double topLeftX, double pixelWidthX, double pixelWidthY, double topLeftY, double pixelHeightX, double pixelHeightY );
    /*!\brief Sets the Georeference coefficients of this image.
     *
     * The coefficients must be given in the following order:
     * [0] : The x-coordinate of the upper left corner of the image
     * [1] : The x-component of the pixel width
     * [2] : The y-component of the pixel width (in the case of rotated images)
     * [3] : The y-coordinate of the upper left corner of the image
     * [4] : The x-component of the pixel height (in the case of rotated images)
     * [5] : The y-component of the pixel height
     * \param coeff A pointer to an array containing the coefficients.
     */
    void setGeoCoefficients( double* coeff );
    /*! \brief Gets the georeferencing coefficients of the image
     *
     * See setGeoCoefficients(double* coeff)
     */
    bool geoCoefficients( double* coeff ) const;
    /*!\brief Loads an ESRI World file with georeferencing information for this dataset
     *
     * The coefficients in the world file will replace any existing coefficients.
     * Call georeferenced() to check whether the dataset already has coefficients
     * \return Returns true on success or false if the world file contained invalid data or the information could not be set.
     */
    bool loadWorldFile( QString filename );

    /*! \brief Set the projection reference string for this dataset
     */
    bool setProjection( QString projString );
    /*!\brief Fetch the projection definition string for this dataset
     */
    QString projection() const;
    bool operator==( QgsRasterDataset& other );

    /*! \brief Gets the current progress of this filter operation*/
    double progress() const;

    void mapToPixel( double mx, double my, int &outx, int &outy )
    {
      double gt[6];
      mDataset->GetGeoTransform( gt );
      double px;
      double py;
      if ( gt[2] + gt[4] == 0 )
      {
        px = ( mx - gt[0] ) / gt[1];
        py = ( my - gt[3] ) / gt[5];
      }
      else
      {
        QgsRasterDataset::ApplyGeoTransform( mx, my, QgsRasterDataset::InvGeoTransform( gt ), px, py );
      }
      outx = int( px + 0.5 );
      outy = int( py + 0.5 );
    }

    static void ApplyGeoTransform( int inx, int iny, double gt[], double &outx, double &outy )
    {
      outx = gt[0] + inx * gt[1] + iny * gt[2];
      outy = gt[3] + inx * gt[4] + iny * gt[5];
    }

    static double* InvGeoTransform( double gt_in[] )
    {

      double det = gt_in[1] * gt_in[5] - gt_in[2] * gt_in[4];

      double inv_det = 1.0 / det;

      double *gt_out = new double[6];
      gt_out[1] =  gt_in[5] * inv_det;
      gt_out[4] = -gt_in[4] * inv_det;

      gt_out[2] = -gt_in[2] * inv_det;
      gt_out[5] =  gt_in[1] * inv_det;
      gt_out[0] = ( gt_in[2] * gt_in[3] - gt_in[0] * gt_in[5] ) * inv_det;
      gt_out[3] = ( -gt_in[1] * gt_in[3] + gt_in[0] * gt_in[4] ) * inv_det;

      return gt_out;
    }

	static double round(double d)
	{
		return floor(d + 0.5);
	}


  protected:
    static bool copyDataset( GDALDataset* srcDs, GDALDataset* destDs );
    GDALDataset* mDataset, *mBaseDataset;
    virtual GDALDataType dataType( int theBand ) const;
    QgsRasterDataset() {}
    void setImageSize( int xSize, int ySize );
    /*! \brief Registers all gdal drivers
     */
    static void registerGdalDrivers();
    //Virtual methods

    /*! \brief Reads a line from the specified raster band
     *
     * Caller has ownership of the allocated memory and is responsible for deallocating it.
     * \return Returns NULL if an invalid band or line index is given.
    */
    virtual void* readLine( int theBand, int line );



  private:
    void initialize();

    void* dataIndex( void* data, int band, int index );
  private:
    template <int bufferCount>
    struct DataBuffer
    {
      DataBuffer(): currentBuffer( 0 )
      {
        for ( int i = 0; i < bufferCount; i++ )
        {
          data[i] = NULL;
          line[i] = -1;
        }
      }
      ~DataBuffer()
      {
        for ( int i = 0; i < bufferCount; i++ )
        {
          if ( data[i] )
          {
            VSIFree( data[i] );
          }
        }
      }
      void* data[bufferCount];
      int line[bufferCount];
      int currentBuffer;
    };

    int mXSize;
    int mYSize;
    bool mFail;
    bool mVirtual;
    //TODO: Or Not to do? What is mScale for again?
    double mScale;
    QString mFilePath;
    QString mTempPath;
    bool mOpen;
    Access mAccess;
    static const int mBufferCount = 2;
    DataBuffer<mBufferCount>* mBuffers;
};

#endif // QGSRASTERDATASET_H
