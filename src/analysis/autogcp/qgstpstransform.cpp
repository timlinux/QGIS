/***************************************************************************
qgstpstransform.cpp - Warps an image using a thin plate spline transform
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
/* $Id: qgstpstransform.cpp 606 2010-10-29 09:13:39Z jamesm $ */


#include "qgstpstransform.h"

GDALTransformerFunc QgsTpsTransform::transformerFunction() const
{
  return GDALTPSTransform;
}
void* QgsTpsTransform::transformerArgs( int nNumGcps, GDAL_GCP* pGcps ) const
{
  void* args = GDALCreateTPSTransformer( nNumGcps, pGcps, false );

  return args;
}

void QgsTpsTransform::destroyTransformer( void* pTransformerArg )
{
  GDALDestroyTPSTransformer( pTransformerArg );
}



