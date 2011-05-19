/***************************************************************************
qgsorthorectificationfilter.h - Performs orthorectification on an image using a sensor model
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
/* $Id: qgsorthorectificationfilter.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSORTHORECTIFICATIONFILTER_H
#define QGSORTHORECTIFICATIONFILTER_H

#include "qgsimagefilter.h"
#include "qgssensormodel.h"
#include "qgssampler.h"
#include "qgselevationmodel.h"

/*! \ingroup analysis
 *
 * \brief This class applies orthorectification to an image using and Rational Polynomial Coefficients (RPC) sensor model.
 *
 * The class can be used as follows:
 * \code
 * QgsRasterDataset* src;
 * QgsRpcModel* model;
 * QgsSampler* sampler;
 *
 * //Get the raw image
 * //Create the model and sampler
 * //...
 *
 * QgsImageFilter* orthoFilter = new QgsOrthorectificationFilter(image, sensorModel, sampler);
 * QgsRasterDataset* orthoImage = orthoFilter->applyFilter();
 * if (orthoImage)
 *   delete orthoImage
 * delete orthoFilter;
 * \endcode
 *
 */
class ANALYSIS_EXPORT QgsOrthorectificationFilter : public QgsImageFilter
{
  public:

    /*! \brief Contructs a QgsOrthorectificationFilter from the given image and sensor model
     * \param image A pointer to the raw input image. This dataset will not be modified.
     * \param sensormodel The sensor model to use for coordinate transformations
     * \param sampler The image re-sampling algorithm to use.
     */
    QgsOrthorectificationFilter( QgsRasterDataset* image, QgsSensorModel* sensormodel, QgsSampler* sampler, QgsElevationModel* DEM = NULL );
    /*!\brief QgsOrthorectification destructor
     * Empty.
     */
    ~QgsOrthorectificationFilter() {};
    /*! \brief Applies the orthorectification to the input image, producing an ortho-image
     * \return A pointer to a new image representing the ortho-image, or NULL on failure
     */
    QgsRasterDataset* applyFilter();

    /**************************************************
     *  Getters and Setters
     **************************************************/
    /*! \brief Gets the QgsSampler to use for sampling of the source image*/
    QgsSampler* sampler() const { return mSampler; }
    /*! \brief Gets the QgsSensorModel to use for converting Ground Coordinates to Raw pixel coordinates
     *
     * @note Currently only the QgsRpcModel is supported
     */
    QgsSensorModel* sensorModel() const { return mSensorModel; }
    /*! \brief Sets the QgsSampler to use for sampling of the source image*/
    void setSampler( QgsSampler* sampler ) { mSampler = sampler; }
    /*! \brief Gets the output drivername*/
    QString driver()const;
    /*! \brief Sets the output driver*/
    void setDriver( QString name );
  private:
    void transformCoordinate( double* coeff, const int c, const int l, double& X, double& Y );
  private:
    QgsSensorModel* mSensorModel;
    QgsSampler* mSampler;
    QgsElevationModel* mDem;
    QString mDriver;
};

inline void QgsOrthorectificationFilter::transformCoordinate( double* coeff, const int c, const int l, double& X, double& Y )
{
  X = coeff[0] + c * coeff[1] + l  * coeff[2];
  Y = coeff[3] + c * coeff[4] + l  * coeff[5];
}

#endif  /* QGSORTHORECTIFICATIONFILTER_H */

