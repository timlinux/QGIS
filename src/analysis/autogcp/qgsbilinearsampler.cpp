/***************************************************************************
qgsbilinearsampler.cpp - Samples an image using bilinear interpolation
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
/* $Id: qgsbilinearsampler.cpp 506 2010-10-16 14:02:25Z jamesm $ */

#include "qgsbilinearsampler.h"

double QgsBilinearSampler::samplePoint( QgsRasterDataset* image, int band, double pixel, double line ) const
{
  if ( image )
  {
    int x1, x2, y1, y2;
    x1 = ( int ) pixel;
    y1 = ( int ) line;
    int width, height;
    image->imageSize( width, height );
    if ( x1 < 0 ||
         y1 < 0 ||
         x1 >= width ||
         y1 >= height )
    {
      return 0.0;
    }
    x2 = x1 + 1;
    y2 = y1 + 1;

    if ( !( band > 0 && band <= image->rasterBands() ) )
    {
      return -1.0;
    }
    double ratiox = ( double ) x2 - pixel;
    double ratioy = ( double ) y2 - line;

    double vala = image->readValue( band, x1, y1 );
    double val2;
    if ( x2 < width )
    {
      val2 = image->readValue( band, x2, y1 );
      vala = ratiox * vala + ( 1.0 - ratiox ) * val2;
    }

    if ( y2 < height )
    {
      double valb = image->readValue( band, x1, y2 );
      if ( x2 < width )
      {
        val2 = image->readValue( band, x2, y2 );
        valb = ratiox * valb + ( 1.0 - ratiox ) * val2;
      }

      vala = ratioy * vala + ( 1.0 - ratioy ) * valb;
    }
    return vala;

  }
  else
  {
    return -1.0;
  }
}
