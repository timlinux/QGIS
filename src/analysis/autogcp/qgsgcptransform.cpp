/***************************************************************************
qgsgcptransform.cpp - Abstract base class for warping images using a set of GCPs
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
/* $Id: qgsgcptransform.cpp 606 2010-10-29 09:13:39Z jamesm $ */


#include "qgsgcptransform.h"
#include "qgslogger.h"

QgsGcpTransform::QgsGcpTransform():
    mDriverName( "" ),
    mSrc( NULL ),
    mDestFile( "" ),
    mGcpSet( NULL ),
    mProgress( 0 )
{

}

QgsGcpTransform::~QgsGcpTransform()
{

}

bool QgsGcpTransform::applyTransform()
{
  if ( NULL == mSrc || mSrc->failed() )
  {
    QgsLogger::debug( "Invalid Source image" );
    return false;
  }
  if ( NULL == mGcpSet || mGcpSet->size() == 0 )
  {
    QgsLogger::debug( "Invalid GCP set" );
    return false;
  }
  if ( mDestFile.isEmpty() )
  {
    QgsLogger::debug( "Invalid output filename" );
    return false;
  }

  if ( !mSrc->reopen() )
  {
    QgsLogger::debug( "Failed to reopen dataset" );
  }
  GDALDataset* pSrcDs = mSrc->gdalDataset();
  typedef QList<QgsGcp*> PointsList;
  const PointsList& gcpList = mGcpSet->constList();
  int nNumGcps = gcpList.size();
  GDAL_GCP* pGcps = new GDAL_GCP[nNumGcps];
  GDALInitGCPs( nNumGcps, pGcps );
  for ( int i = 0; i < nNumGcps; ++i )
  {
    QgsGcp* srcGcp = gcpList.at( i );
    GDAL_GCP& gcp = pGcps[i];
    int pixel, line;
    double geoX = srcGcp->rawX(),
                  geoY = srcGcp->rawY();
    mSrc->transformCoordinate( pixel, line, geoX, geoY, QgsRasterDataset::TM_GEOPIXEL );
    gcp.dfGCPLine = line;//geoX;
    gcp.dfGCPPixel = pixel;//-geoY;
    gcp.dfGCPX = srcGcp->refX();
    gcp.dfGCPY = srcGcp->refY();
    gcp.dfGCPZ = 0;
    gcp.pszInfo = NULL;
  }


//this->transformerArgs(nNumGcps,pGcps);

  void* pTransformerArgs = this->transformerArgs( nNumGcps, pGcps );
  GDALTransformerFunc pfnTransformerFunc = this->transformerFunction();

  double adfGeoTransform[6];
  int nXSize, nYSize;
  CPLErr result = GDALSuggestedWarpOutput( pSrcDs, pfnTransformerFunc, pTransformerArgs, adfGeoTransform, &nXSize, &nYSize );
  if ( CE_None != result )
  {
    QgsLogger::debug( "Failed to create suggested warp output" );
    return NULL;
  }

  GDALDataset* pDstDs = this->createDestDataset( pSrcDs, pfnTransformerFunc, pTransformerArgs, nXSize, nYSize, adfGeoTransform );
  if ( NULL == pDstDs )
  {
    return false;
  }
  //Create Warp options
  if ( NULL == pTransformerArgs )
  {
    QgsLogger::debug( "Failed to create TPS transformer" );
    return false;
  }

  GDALWarpOptions* pWarpOptions = GDALCreateWarpOptions();
  pWarpOptions->hSrcDS = ( GDALDatasetH ) pSrcDs;
  pWarpOptions->hDstDS = ( GDALDatasetH )pDstDs;
  pWarpOptions->pfnProgress = (GDALProgressFunc) updateProgress;
  pWarpOptions->pProgressArg = ( void* )this;
  pWarpOptions->pTransformerArg = wrapTransformerArgs( pfnTransformerFunc, pTransformerArgs, adfGeoTransform );
  pWarpOptions->pfnTransformer = GeoToPixelTransform;
  pWarpOptions->eResampleAlg = GRA_Bilinear;
  pWarpOptions->nBandCount = pSrcDs->GetRasterCount();
  pWarpOptions->panSrcBands =
    ( int * ) CPLMalloc( sizeof( int ) * pWarpOptions->nBandCount );
  pWarpOptions->panDstBands =
    ( int * ) CPLMalloc( sizeof( int ) * pWarpOptions->nBandCount );
  for ( int i = 0; i < pWarpOptions->nBandCount; ++i )
  {
    pWarpOptions->panSrcBands[i] = i + 1;
    pWarpOptions->panDstBands[i] = i + 1;
  }

  /*TransformChain *chain = new TransformChain;
  chain->GDALTransformer = pfnTransformerFunc;
  chain->GDALTransformerArg = pTransformerArgs;
  double adfGeotransform[6];
  pDstDs->GetGeoTransform(adfGeotransform);
  memcpy( chain->adfGeotransform, adfGeotransform, sizeof( double )*6 );
  // TODO: In reality we don't require the full homogeneous matrix, so GeoToPixelTransform and matrix inversion could
  // be optimized for simple scale+offset if there's the need (e.g. for numerical or performance reasons).
  GDALInvGeoTransform( chain->adfGeotransform, chain->adfInvGeotransform );
  pWarpOptions->pTransformerArg = ( void* )chain;
  */

  GDALWarpOperation operation;
  result = operation.Initialize( pWarpOptions );
  result = operation.ChunkAndWarpImage( 0, 0, nXSize, nYSize );

//  delete chain;
  this->destroyTransformer( pTransformerArgs );
//
  GDALDestroyWarpOptions( pWarpOptions );
  GDALClose( pDstDs );

  return ( result == CE_None );
}


GDALDataset* QgsGcpTransform::createDestDataset( GDALDatasetH hSrcDs, const GDALTransformerFunc& pfnTransformerFunc, void* pTransformerArgs, int nXSize, int nYSize, double* adfGeoTransform )
{

  QString driverName = driver();
  GDALDriver* pDriver = NULL;
  if ( !driverName.isEmpty() && !driverName.isNull() )
  {
    pDriver = ( GDALDriver* ) GDALGetDriverByName( driverName.toLatin1().data() );
  }
  GDALDataset* pSrcDs = ( GDALDataset* )hSrcDs;
  if ( NULL == pDriver )
  {
    pDriver = pSrcDs->GetDriver();
  }

  int nBands = pSrcDs->GetRasterCount();
  GDALDataType eType = pSrcDs->GetRasterBand( 1 )->GetRasterDataType();
  GDALDataset* pDstDs = pDriver->Create( mDestFile.toLatin1().data(), nXSize, nYSize, nBands, eType, NULL );
  if ( NULL == pDstDs )
  {
    QgsLogger::debug( "Failed to create output dataset" );
    return NULL;
  }
  pDstDs->SetGeoTransform( adfGeoTransform );
  pDstDs->SetProjection( pSrcDs->GetProjectionRef() );

  return pDstDs;
}

int QgsGcpTransform::updateProgress( double dfComplete, const char*, void* pProgressArg )
{
  QgsGcpTransform* transform = static_cast<QgsGcpTransform*>( pProgressArg );
  transform->setProgress( dfComplete );
  return 1;
}

void QgsGcpTransform::setProgress( double dComplete )
{

  mProgress = dComplete;
}
double QgsGcpTransform::progress()const
{

  return mProgress;
}

void* QgsGcpTransform::wrapTransformerArgs( GDALTransformerFunc GDALTransformer, void *GDALTransformerArg, double *padfGeotransform ) const
{
  TransformChain *chain = new TransformChain;
  chain->GDALTransformer = GDALTransformer;
  chain->GDALTransformerArg = GDALTransformerArg;
  memcpy( chain->adfGeotransform, padfGeotransform, sizeof( double )*6 );
  // TODO: In reality we don't require the full homogeneous matrix, so GeoToPixelTransform and matrix inversion could
  // be optimized for simple scale+offset if there's the need (e.g. for numerical or performance reasons).
  if ( !GDALInvGeoTransform( chain->adfGeotransform, chain->adfInvGeotransform ) )
  {
    // Error handling if inversion fails - although the inverse transform is not needed for warp operation
    return NULL;
  }
  return ( void* )chain;
}

int QgsGcpTransform::GeoToPixelTransform( void *pTransformerArg, int bDstToSrc, int nPointCount,
    double *x, double *y, double *z, int *panSuccess )
{
  TransformChain *chain = static_cast<TransformChain*>( pTransformerArg );
  if ( chain == NULL )
  {
    return false;
  }

  if ( !bDstToSrc )
  {
    // Transform to georeferenced coordinates
    if ( !chain->GDALTransformer( chain->GDALTransformerArg, bDstToSrc, nPointCount, x, y, z, panSuccess ) )
    {
      return false;
    }
    // Transform from georeferenced to pixel/line
    for ( int i = 0; i < nPointCount; ++i )
    {
      if ( !panSuccess[i] ) continue;
      double xP = x[i];
      double yP = y[i];
      x[i] = chain->adfInvGeotransform[0] + xP * chain->adfInvGeotransform[1] + yP * chain->adfInvGeotransform[2];
      y[i] = chain->adfInvGeotransform[3] + xP * chain->adfInvGeotransform[4] + yP * chain->adfInvGeotransform[5];
    }
  }
  else
  {
    // Transform from pixel/line to georeferenced coordinates
    for ( int i = 0; i < nPointCount; ++i )
    {
      double P = x[i];
      double L = y[i];
      x[i] = chain->adfGeotransform[0] + P * chain->adfGeotransform[1] + L * chain->adfGeotransform[2];
      y[i] = chain->adfGeotransform[3] + P * chain->adfGeotransform[4] + L * chain->adfGeotransform[5];
    }
    // Transform from georeferenced coordinates to source pixel/line
    if ( !chain->GDALTransformer( chain->GDALTransformerArg, bDstToSrc, nPointCount, x, y, z, panSuccess ) )
    {
      return false;
    }
  }
  return true;
}
