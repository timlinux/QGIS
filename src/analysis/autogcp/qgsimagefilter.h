/***************************************************************************
qgsimagefilter.h - Abstract base class for a transformation filter that can be
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
/* $Id: qgsimagefilter.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSIMAGETRANSFORM_H
#define QGSIMAGETRANSFORM_H

#include "qgsrasterdataset.h"
#include "qgsabstractoperation.h"
/*! \ingroup analysis
 *  Abstract base class for any filter that can be applied to an image
 */
class ANALYSIS_EXPORT QgsImageFilter : public QgsAbstractOperation
{
  public:
    virtual ~QgsImageFilter();
    /*! \brief Applies this filter to the given image*/
    virtual QgsRasterDataset* applyFilter() = 0;
    /*! \brief Gets the destination filename of the output image*/
    virtual QString destinationFile() const { return mDestFile; }
    /*! \brief Sets the destination filename of the output image*/
    void setDestinationFile( QString path ) { mDestFile = path; }
    /*! \brief Gets the current progress of this filter operation*/
    double progress() const;
    /*! \brief Sets the current progress of this filter operation*/
    void setProgress( double dProg );
  protected:
    /*!\brief Constructs an image filter with the given source image
     */
    QgsImageFilter( QgsRasterDataset* image );

    QgsRasterDataset* mSrc;
    QString mDestFile;
    double mProgress;

};

#endif  /* QGSIMAGETRANSFORM_H */

