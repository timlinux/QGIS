/***************************************************************************
qgsbilinearsampler.h - Samples an image using bilinear interpolation
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
/* $Id: qgsbilinearsampler.h 506 2010-10-16 14:02:25Z jamesm $ */
#ifndef QGSBILINEARSAMPLER_H
#define QGSBILINEARSAMPLER_H

#include "qgssampler.h"

class ANALYSIS_EXPORT QgsBilinearSampler : public QgsSampler
{
  public:
    /*! \brief Samples an image at a given location using bilinear interpolation
     * Edge pixels are treated as exceptions and uses either linear filtering or nearest neighbour
     * \param image A pointer to the image to sample from
     * \param band The band inside the image to sample from
     * \param pixel The x coordinate in pixels
     * \param line The y-coordinate in pixels
     * \return The sampled value obtained through the necessary interpolation or 0.0 if the pixel/line coordinate is out of bounds
     */
    double samplePoint( QgsRasterDataset* image, int band, double pixel, double line )const;


};

#endif  /* QGSBILINEARSAMPLER_H */

