/***************************************************************************
qgsbilinearsampler.cpp - Samples an image using bilinear interpolation
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
/* $Id: qgspointsampler.cpp 606 2010-10-29 09:13:39Z jamesm $ */

#include "qgspointsampler.h"

double QgsPointSampler::samplePoint( QgsRasterDataset* image, int band, double pixel, double line ) const
{
  if ( image && band > 0 && band <= image->rasterBands() )
  {
    if ( pixel < 0 ||
         line < 0 ||
         pixel >= image->imageXSize() ||
         line >= image->imageYSize() )
    {
      return 0.0;
    }
    return image->readValue( band, pixel, line );

  }
  else
  {
    return -1.0;
  }
}
