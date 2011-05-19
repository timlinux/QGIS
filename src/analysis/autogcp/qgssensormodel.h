/***************************************************************************
qgssensormodel.h - Abstract base class for all sensor models. Provides the
 interface for transforming Ground coordinates to image coordinates
--------------------------------------
Date : 15-September-2010
Author : James Meyer
Copyright : (C) 2010 by C.J. Meyer
Email : jamesmeyerx at gmail.com

***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgssensormodel.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSSENSORMODEL_H
#define QGSSENSORMODEL_H

#include "qgspoint.h"

/*!\ingroup analysis
 *
 * Abstract base class for all sensor models. Provides the
 * interface for transforming Ground coordinates to image coordinates
 */
class ANALYSIS_EXPORT QgsSensorModel
{
  public:
    /*!
     * \brief QgsSensorModel destructor
     * Abstract
     */
    virtual ~QgsSensorModel() {};
    /*!\brief Returns the normalised row position for pixel at X,Y,Z
     *
     */
    virtual double r( double X, double Y, double Z ) = 0;
    /*!\brief Returns the normalised column position for pixel at X,Y,Z
     */
    virtual double c( double X, double Y, double Z ) = 0;

    //virtual double calculateNewPosition(double X, double Y, double Z, QgsPoint *newPoint) = 0;
};



#endif /* QGSSENSORMODEL_H */

