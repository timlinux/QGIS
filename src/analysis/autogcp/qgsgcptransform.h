/***************************************************************************
qgsgcptransform.h - Abstract base class for warping images using a set of GCPs
--------------------------------------
Date : 23-October-2010
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
/* $Id: qgsgcptransform.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSGCPTRANSFORM_H
#define QGSGCPTRANSFORM_H

#include "qgsrasterdataset.h"
#include "qgsgcpset.h"
#include <QString>
#include <gdal_alg.h>
#include <gdal.h>
#include <gdalwarper.h>
#include "qgsabstractoperation.h"
/*! \ingroup analysis
 *
 * \brief Abstract base class for GCP-based raster transformations
 *
 * The QgsGcpTransform class is an abtraction of all the operations common to
 * GCP-based raster transformations using GDAL transformer functions.
 */
class ANALYSIS_EXPORT QgsGcpTransform : public QgsAbstractOperation
{
  public:

    QgsGcpTransform();
    ~QgsGcpTransform();

    /*! \brief Applies the GcpTransform
     *
     * If successful, this function will generate an output raster with parameters set before hand.
     *
     * \return True on success, false otherwise
     */
    bool applyTransform();

    /*****************************************
     * Getters & Setters
     *****************************************/
    /*! \brief Sets the filename for the output raster*/
    void setDestinationFile( QString filename ) {mDestFile = filename;}
    /*! \brief Gets the filename for the output raster*/
    QString destinationFile() const {return mDestFile;}

    /*! \brief Sets the source raster*/
    void setSourceImage( QgsRasterDataset* pSrcDs ) {mSrc = pSrcDs;}
    /*! \brief Gets the source raster*/
    QgsRasterDataset* sourceImage() const {return mSrc;}

    /*! \brief sets the QgsGcpSet to use as input to the transformation*/
    void setGcpSet( QgsGcpSet* pGcpSet ) {mGcpSet = pGcpSet;}
    /*! \brief Gets a pointer to the QgsGcpSet*/
    QgsGcpSet* gcpSet() const {return mGcpSet;}

    /*! \brief Sets the driver to use for the output raster*/
    void setDriver( QString driverName ) {mDriverName = driverName;}
    /*! \brief Gets the driver name that will be used for output rasters*/
    QString driver()const {return mDriverName;}

    /*!\brief Sets the current progress of this transform operation*/
    void setProgress( double dComplete );
    /*! \brief Gets the current progress of this transform operation*/
    double progress()const;

  protected:
    virtual GDALTransformerFunc transformerFunction() const = 0;
    virtual void* transformerArgs( int nNumGcps, GDAL_GCP* pGcps ) const = 0;
    virtual void destroyTransformer( void* ) = 0;
  private:
    struct TransformChain
    {
      GDALTransformerFunc GDALTransformer;
      void *              GDALTransformerArg;
      double              adfGeotransform[6];
      double              adfInvGeotransform[6];
    };
    void* wrapTransformerArgs( GDALTransformerFunc GDALTransformer, void *GDALTransformerArg, double *padfGeotransform ) const;
    GDALDataset* createDestDataset( GDALDatasetH hSrcDs, const GDALTransformerFunc& pfnTransformerFunc, void* pTransformerArgs, int xSize, int ySize, double* adfGeoTransform );
    static int updateProgress( double dfComplete, const char*, void* pProgressArg );
    static int GeoToPixelTransform( void *pTransformerArg, int bDstToSrc, int nPointCount, double *x, double *y, double *z, int *panSuccess );
  private:
    QgsRasterDataset* mSrc;
    QgsGcpSet* mGcpSet;
    QString mDestFile;
    QString mDriverName;

    double mProgress;


};

#endif  /* QGSGCPTRANSFORM_H */

