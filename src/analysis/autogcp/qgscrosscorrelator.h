/***************************************************************************
qgscrosscorrelator.cpp - This class handles the cross correlation of two
subimages.
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
/* $Id: qgscrosscorrelator.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSCROSSCORRELATOR_H
#define QGSCROSSCORRELATOR_H

#include "qgsrasterdataset.h"

/*! \ingroup analysis
 *
 * \brief Class calculates the similarity between two sub-images
 *
 * The class uses normalized cross-correlation to compare two images.
 * Currently it does not support scaling.
 */
class ANALYSIS_EXPORT QgsCrossCorrelator
{
  public:
    /**\brief QgsCrossCorrelator Constructor
    Constructs the correlator with the given chip images
    */
    QgsCrossCorrelator( QgsRasterDataset* ref, QgsRasterDataset* raw );
    /**\brief QgsCrossCorrelator Destructor
    Destroys the QgsCrossCorrelator instance, releasing any owned resources.
    */
    virtual ~QgsCrossCorrelator();

    /*! \brief Calculates the correlation coefficient between two buffers of raster data
     *
     * The cross-correlation is done with minimal error checking for performance.
     * The caller of the function is responsible for ensuring that the dimensions match the data in the buffers.
     */
    static double correlationCoefficent( double* srcData, double* destData, int xSize, int ySize );

    /*! \brief returns the calculated correlation coefficient of the cross-correlator object*/
    double correlationValue();
    typedef  int Scheme;
    static const Scheme GREY = 0;
    static const Scheme RGB = 1;
    static const Scheme CMYK = 2;

  private:
    Scheme refConvert, rawConvert;
    QgsRasterDataset* refChip, * rawChip;
    double refAvg, rawAvg;
    void calculateAverages();
    double calculateCC();
    double greyLevel( bool isRef, int x, int y );
    int greyRefBand, redRefBand, blueRefBand, greenRefBand, cyanRefBand, magentaRefBand, yellowRefBand, blackRefBand;
    int greyRawBand, redRawBand, blueRawBand, greenRawBand, cyanRawBand, magentaRawBand, yellowRawBand, blackRawBand;
};

#endif // QGSCROSSCORRELATOR_H
