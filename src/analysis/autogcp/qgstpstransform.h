/***************************************************************************
qgstpstransform.h - Warps an image using a thin plate spline transform provided by GDAL
--------------------------------------
Date : 23-October-2010
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
/* $Id: qgstpstransform.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSTPSTRANSFORM_H
#define QGSTPSTRANSFORM_H
#include "qgsgcptransform.h"
/*! \ingroup analysis
 *
 * \brief Warps an image using a thin plate spline transform provided by GDAL
 */
class ANALYSIS_EXPORT QgsTpsTransform : public QgsGcpTransform
{
  public:
    ~QgsTpsTransform() {}
  protected:

    GDALTransformerFunc transformerFunction() const;
    void* transformerArgs( int nNumGcps, GDAL_GCP* pGcps ) const;
    void destroyTransformer( void* );
  private:





};


#endif  /* QGSTPSTRANSFORM_H */

