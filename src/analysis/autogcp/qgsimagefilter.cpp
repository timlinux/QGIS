/***************************************************************************
qgsimagefilter.cpp - Abstract base class for a transformation filter that can be
 applied to an image
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
/* $Id: qgsimagefilter.cpp 606 2010-10-29 09:13:39Z jamesm $ */

#include "qgsimagefilter.h"
#include "qgsimageanalyzer.h"
#include <QFile>

QgsImageFilter::~QgsImageFilter()
{

}

QgsImageFilter::QgsImageFilter( QgsRasterDataset* image )
    : mSrc( image ),
    mProgress( 0.0 )
{
  if ( mSrc )
  {
    QString srcFile = mSrc->filePath();
    mDestFile = "ORTHO_" + srcFile;
  }
}

double QgsImageFilter::progress() const
{
  return mProgress;
}
void QgsImageFilter::setProgress( double dProg )
{
  mProgress = dProg;
}
