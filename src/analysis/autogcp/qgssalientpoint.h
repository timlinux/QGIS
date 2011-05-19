/**************************************************************************
qgssalientpoint.h - Represents a 2D point of saliency in pixel space
--------------------------------------
Date : 07-May-2010
Copyright : (C) 2010 by FoxHat Solutions
Author: James Meyer
Email : jamesmeyerx@gmail.com
***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgssalientpoint.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSSALIENTPOINT_H
#define QGSSALIENTPOINT_H

#include "qgspoint.h"
/*! \ingroup analysis
 *
  This class represents any point of interest on a two-dimensional raster image.
*/
class ANALYSIS_EXPORT QgsSalientPoint : public QgsPoint
{
  public:
    /*! \brief QgsSalientPoint constructor
    constructs a feature point at the given x, y coordinates and optionally a saliency value.
    */
    QgsSalientPoint( double x = 0, double y = 0, double theSaliency = 0 ):
        QgsPoint( x, y ),
        mSaliency( theSaliency )
    {
    }

    /*! \brief Gets the Saliency value of this feature.
    */
    double saliency()const { return mSaliency; }

    /*! \brief Sets the Saliency value of this feature.
    */
    void setSaliency( double value )  {  mSaliency = value;  }

  private:
    double mSaliency;
};

#endif // QGSSALIENTPOINT_H
