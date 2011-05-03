/***************************************************************************
qgssampler.h - Abstract class that samples from an image at non-grid pixel coordinates
--------------------------------------
Date : 14-September-2010
Author : James Meyer
Copyright : (C) 2010 by C.J. Meyer
Email : jamesmeyerx@gmail.com

/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgssampler.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSSAMPLER_H
#define QGSSAMPLER_H

#include "qgsrasterdataset.h"
/*!\ingroup analysis
 *
 * Abstract base class of all image (re)samplers
 * An image sampler is used to sample pixels from an image based on coordinates that are not exact grid coordinates
 *
 */
class ANALYSIS_EXPORT QgsSampler
{
  public:
    virtual ~QgsSampler() {}
    virtual double samplePoint( QgsRasterDataset* image, int band, double pixel, double line ) const = 0;
};


#endif  /* QGSSAMPLER_H */

