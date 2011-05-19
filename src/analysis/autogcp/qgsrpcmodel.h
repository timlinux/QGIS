/***************************************************************************
qgsrpcmodel.h - This class encapsulates a Rational Polynomial Coefficients
model, or RFM (Rational Function Model), which is constructed using GCPs, hence
it is terrain-dependent. Its major public methods return values indicating the
orthorectified normalised pixel position for each pixel in the original image.
--------------------------------------
Date : 12-July-2010
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
/* $Id: qgsrpcmodel.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSRPCMODEL_H
#define QGSRPCMODEL_H

#include "qgssensormodel.h"
#include "qgselevationmodel.h"
#include "qgsgcpset.h"
#include "qgsrasterdataset.h"
#include "qgsmatrix.h"
#include <vector>
#define RMSE_THRESHOLD 0.5
#define GCP_MINIMUM 16

/*! \ingroup analysis
 *
 * This class encapsulates a Rational Polynomial Coefficients
 * model, or RFM (Rational Function Model), which is constructed using GCPs, hence
 * it is terrain-dependent. Its major public methods return values indicating the
 * orthorectified normalised pixel position for each pixel in the original image.
 */
class ANALYSIS_EXPORT QgsRpcModel : public QgsSensorModel
{
  public:
    /**\brief QgsRPCModel Constructor
     *
     * Constructs the RPC model with the given GCPs
     * \param gcpSet The GCP set to use for construction of the model
     * \param img The raw image for which the sensor model is constructed
     * \param dem An optional Digital elevation model for higher accuracy
     */
    QgsRpcModel( QgsGcpSet* gcpSet, QgsRasterDataset* img, QgsElevationModel* dem = NULL );

    /**\brief QgsRPCModel Destructor
       Destroys the QgsRPCModel instance, releasing any owned resources.
       */
    virtual ~QgsRpcModel();
    bool constructModel();
    /**\brief
       Returns the normalised row position for pixel at X,Y,Z (geographical ground coordinates)
       */
    double r( double X, double Y, double Z );

    /**\brief
       Returns the normalised column position for pixel at X,Y,Z (geographical ground coordinates)
       */
    double c( double X, double Y, double Z );

    /*! \brief Sets the RMS Error threshold for GCPs used in the model
     *
     * The RMSE is calculated in pixel distance.
     * As long as the RMSE for this model is above the threshold, the GCP with highest
     * deviation between predicted and calculated pixel and line values
     * will be removed from the list, and the model recalculated.
     *
     */
    void setRmsErrorThreshold( double dThreshold )
    {
      if ( mThreshold >= 0 )
      {
        mThreshold = dThreshold;
      }
    }
    /*! \brief Gets the current RMSE threshold
     *
     * \sa setRmsErrorThreshold()
     */
    double rmsErrorThreshold()const
    {
      return mThreshold;
    }
    /*! \brief Gets the current total RMSE of the model
     *
     * \sa rmsErrorThreshold()
     * \sa setRmsErrorThreshold()
     */
    double rmsError() const { return mRMSE; }

    void setGcpSet( QgsGcpSet* gcpSet )
    {
      while ( !mGcpSet->list().isEmpty() )
      {
        delete( mGcpSet->list().takeAt( 0 ) );
      }
      mGcpSet = gcpSet;
    }
  private:
    /*! \brief calculates the new RMSE for the model.
     *
     * \param pHighestDev The address of a pointer to a QgsGcp where the highest deviating GCPs will be returned
     * \param fHighestDev A pointer to a double where the highest deviation value will be returned
     * \return The current RMS Error of the model
     *
     * \sa rmsError()
     */
    double calculateRmsError( QgsGcp** pHighestDev, double* fHighestDev );
    typedef bool( QgsRpcModel::*InitFunction )();
    InitFunction mInit;
    bool initialize();
    bool initializeNoDem();
    double J[39];
    double K[39];
    double j[11];
    double k[11];
    bool mInitialized;
    bool mUseDem;
    QgsRasterDataset* mImageData;
    QgsElevationModel* mDem;
    double mGeoTopLeftX, mGeoTopLeftY;
    double mGeoBottomRightX, mGeoBottomRightY;
    double mGeoWidth, mGeoHeight;
    double mImageXSize, mImageYSize;
    double mRMSE;
    QgsGcpSet* mGcpSet;
    double mThreshold;
};

#endif // QGSRPCMODEL_H
