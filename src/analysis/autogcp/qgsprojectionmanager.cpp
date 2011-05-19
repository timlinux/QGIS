/***************************************************************************
qgsprojectionmanager.cpp - The ProjectionManager class is responsible for the
projection of GCPs using the projection information either contained within the
Metadata of the image, or as specified by the user.
This class uses the GDAL Warp library to perform the projection on the provided
GCP chip.
--------------------------------------
Date : 07-May-2010
Copyright : (C) 2010 by FoxHat Solutions
Author: Francois Maass
***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsprojectionmanager.cpp 606 2010-10-29 09:13:39Z jamesm $ */
#include <algorithm>
#include "qgsprojectionmanager.h"
#include "qgscoordinatereferencesystem.h"

QgsProjectionManager::QgsProjectionManager( QgsRasterDataset *refImageDataset, QgsRasterDataset *rawImageDataset )
{
  if (( refImageDataset != NULL ) && ( rawImageDataset != NULL ) )
  {

    mRefDataset = refImageDataset->gdalDataset();
    mRawDataset = rawImageDataset->gdalDataset();
    if (( mRefDataset ) && ( mRawDataset ) )
    {
      mFail = false;
    }
    else
    {
      mFail = true;
      QgsLogger::debug( "QGisProjectionManager cannot have NULL-valued datasets." );
    }

  }
  else
  {
    mFail = true;
    QgsLogger::debug( "QGisProjectionManager cannot have NULL-valued datasets." );
  }

}

QgsProjectionManager::QgsProjectionManager()
{
  mRefDataset = NULL;
  mRawDataset = NULL;
  mFail = false;
}

void QgsProjectionManager::setReferenceImage( QgsRasterDataset *refImageDataset )
{
  if ( mRefDataset )
  {
    delete mRefDataset;
  }
  mRefDataset = refImageDataset->gdalDataset();
}

void QgsProjectionManager::setRawImage( QgsRasterDataset *rawImageDataset )
{
  if ( mRawDataset )
  {
    delete mRawDataset;
  }
  mRawDataset = rawImageDataset->gdalDataset();
}

QgsRasterDataset* QgsProjectionManager::referenceImage()
{
  QgsRasterDataset *ds = new QgsRasterDataset( mRefDataset );
  return ds;
}

QgsRasterDataset* QgsProjectionManager::rawImage()
{
  QgsRasterDataset *ds = new QgsRasterDataset( mRawDataset );
  return ds;
}

QgsProjectionManager::~QgsProjectionManager()
{
  mRefDataset = NULL;
  mRawDataset = NULL;
  delete( mRefDataset );
  delete( mRawDataset );
}

void QgsProjectionManager::notifyFailure( QString message )
{
  mFail = true;
  QgsLogger::debug( message );
}

bool QgsProjectionManager::projectGCPChip( QgsRasterDataset* chipIn, QgsRasterDataset* chipOut )
{
  if ( mFail )
  {
    QgsLogger::debug( "Cannot execute QgsProjectionManager::projectGCP: QgsProjectionManager not properly constructed." );
    return false;
  }

  mFail = false;

  if ( chipIn == NULL )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() - GCP IN pointer invalid!" );
    return false;
  }

  if ( chipOut == NULL )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCPChip() - GCP OUT pointer invalid!" );
    return false;
  }

  GDALDriverH driver;
  GDALDataType outDataType;
  GDALDataset *outputDS;

  //Get the datatype from the first input band to create output datatype
  outDataType = GDALGetRasterDataType( GDALGetRasterBand( mRefDataset, 1 ) );

  //Set the output driver to the same is the mRawDataset's driver
  driver = mRawDataset->GetDriver();
  if ( driver == NULL )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to get output driver." );
    return false;
  }

  //Set source and destination coordinate systems
  const char *srcCoordSystem, *dstCoordSystem = NULL;
  srcCoordSystem = GDALGetProjectionRef( mRefDataset );


  if ( srcCoordSystem == NULL || strlen( srcCoordSystem ) <= 0 )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to get projection definition from Reference Image Dataset" );
    return false;
  }

  dstCoordSystem = GDALGetProjectionRef( mRawDataset );
  if ( dstCoordSystem == NULL || strlen( dstCoordSystem ) <= 0 )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to get projection definition from Destination Image Dataset" );
    return false;
  }


  //Create transformer and get approximate output bounds and resolution
  void *transformArg;

  GDALDataset *chipdataset = chipIn->gdalDataset();

  transformArg = GDALCreateGenImgProjTransformer( chipdataset, srcCoordSystem, NULL, dstCoordSystem, FALSE, 0 , 1 );
  if ( transformArg == NULL )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to create transform arguments." );
    return false;
  }

  double dstGeoTransform[6];
  int nPixels = 0;
  int nLines = 0;
  CPLErr error;
  //Returns the suggested size and resolution of output image, as well as the suggested pixelWidth(nPixels) and pixelHeight(nLines)
  error = GDALSuggestedWarpOutput( chipdataset, GDALGenImgProjTransform, transformArg, dstGeoTransform, &nPixels, &nLines );
  if ( error != CE_None )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCPChip() could not determine suggested transformation" );
    return false;
  }
  GDALDestroyGenImgProjTransformer( transformArg );

  //Create output file --->>>>>>>><<<<<<<<---

  int chipDatasize = nPixels * nLines * GDALGetRasterCount( mRefDataset ) * ( GDALGetDataTypeSize( outDataType ) / 8 );
  void *chipData = CPLMalloc( chipDatasize );
  if ( !chipData )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCPChip() failed to create output chipData." );
    return false;
  }
  char nameString[200];
  sprintf( nameString, "MEM:::DATAPOINTER=%p,PIXELS=%d,LINES=%d,BANDS=%d,DATATYPE=%s",
           chipData, nPixels, nLines, GDALGetRasterCount( mRefDataset ), GDALGetDataTypeName( outDataType ) );

  --  outputDS = chipOut->gdalDataset();
  if ( outputDS == NULL )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCPChip() failed to create output datastet." );
    return false;
  }

  //Set Projection info for new image
  GDALSetProjection( outputDS, dstCoordSystem );

  GDALSetGeoTransform( outputDS, dstGeoTransform );

  //Set color table if it is available and required
  GDALColorTableH colorTable;
  colorTable = GDALGetRasterColorTable( GDALGetRasterBand( mRefDataset, 1 ) );
  if ( colorTable != NULL )
    GDALSetRasterColorTable( GDALGetRasterBand( outputDS, 1 ), colorTable );

  //Get the warp options
  GDALWarpOptions *warpOptions = getWarpOptions( mRefDataset, outputDS, true );

  //Start the warp operation
  warpDatasets( warpOptions, outputDS );

  //cleanup process
  destroyWarpData( warpOptions );

}

void QgsProjectionManager::projectGCP( QgsGcp *originalGCP )
{
  if ( mFail )
  {
    QgsLogger::debug( "Cannot execute QgsProjectionManager::projectGCP: QgsProjectionManager not properly constructed." );
    return;
  }

  if ( originalGCP == NULL )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() - GCP pointer invalid!" );
    return;
  }

  GDALDriverH driver;
  GDALDataType outDataType;
  GDALDataset *outputDS;

  //Get the datatype from the first input band to create output datatype
  outDataType = GDALGetRasterDataType( GDALGetRasterBand( mRefDataset, 1 ) );

  //Set the output driver to the same is the mRawDataset's driver
  driver = mRawDataset->GetDriver();
  if ( driver == NULL )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to get output driver." );
    return;
  }

  //Set source and destination coordinate systems
  const char *srcCoordSystem, *dstCoordSystem = NULL;
  srcCoordSystem = GDALGetProjectionRef( mRefDataset );


  if ( srcCoordSystem == NULL || strlen( srcCoordSystem ) <= 0 )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to get projection definition from Reference Image Dataset" );
    return;
  }

  dstCoordSystem = GDALGetProjectionRef( mRawDataset );
  if ( dstCoordSystem == NULL || strlen( dstCoordSystem ) <= 0 )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to get projection definition from Destination Image Dataset" );
    return;
  }


  //Create transformer and get approximate output bounds and resolution
  void *transformArg;
  QgsRasterDataset *gcpImageDS = originalGCP->gcpChip();


  if ( gcpImageDS == NULL )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() - GCP Chip datset empty (NULL POINTER)" );
    return;
  }
  GDALDataset *chipdataset = gcpImageDS->gdalDataset();


  transformArg = GDALCreateGenImgProjTransformer( chipdataset, srcCoordSystem, NULL, dstCoordSystem, FALSE, 0 , 1 );
  if ( transformArg == NULL )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to create transform arguments." );
    return;
  }

  double dstGeoTransform[6];
  int nPixels = 0;
  int nLines = 0;
  CPLErr error;
  //Returns the suggested size and resolution of output image, as well as the suggested pixelWidth(nPixels) and pixelHeight(nLines)
  error = GDALSuggestedWarpOutput( gcpImageDS->gdalDataset(), GDALGenImgProjTransform, transformArg, dstGeoTransform, &nPixels, &nLines );
  if ( error != CE_None )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() could not determine suggested transformation" );
    return;
  }
  GDALDestroyGenImgProjTransformer( transformArg );

  //Create output file --->>>>>>>><<<<<<<<---

  int chipDatasize = nPixels * nLines * GDALGetRasterCount( mRefDataset ) * ( GDALGetDataTypeSize( outDataType ) / 8 );
  void *chipData = CPLMalloc( chipDatasize );
  if ( !chipData )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to create output chipData." );
    return;
  }
  char nameString[200];
  sprintf( nameString, "MEM:::DATAPOINTER=%p,PIXELS=%d,LINES=%d,BANDS=%d,DATATYPE=%s",
           chipData, nPixels, nLines, GDALGetRasterCount( mRefDataset ), GDALGetDataTypeName( outDataType ) );

  outputDS = ( GDALDataset * ) GDALOpen( nameString, GA_Update );
  if ( outputDS == NULL )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to create output datastet." );
    return;
  }

  //Set Projection info for new image
  GDALSetProjection( outputDS, dstCoordSystem );

  GDALSetGeoTransform( outputDS, dstGeoTransform );

  //Set color table if it is available and required
  GDALColorTableH colorTable;
  colorTable = GDALGetRasterColorTable( GDALGetRasterBand( mRefDataset, 1 ) );
  if ( colorTable != NULL )
    GDALSetRasterColorTable( GDALGetRasterBand( outputDS, 1 ), colorTable );

  //Get the warp options
  GDALWarpOptions *warpOptions = getWarpOptions( mRefDataset, outputDS, true );

  //Start the warp operation
  warpDatasets( warpOptions, outputDS );

  //cleanup process
  destroyWarpData( warpOptions );

  //Set output chip
  QgsRasterDataset *outputQgsDataset;
  outputQgsDataset = new QgsRasterDataset( outputDS );
  originalGCP->setGcpChip( outputQgsDataset );
}

GDALWarpOptions* QgsProjectionManager::getWarpOptions( GDALDataset *srcDataset, GDALDataset *dstDataset, bool progressBar )
{
  GDALWarpOptions *pswops = GDALCreateWarpOptions();
  //set src and dst datasets
  pswops->hSrcDS = srcDataset;
  pswops->hDstDS = dstDataset;

  //set Band options for src and dst datasets
  pswops->nBandCount = 1;
  pswops->panSrcBands = ( int * ) CPLMalloc( sizeof( int ) * pswops->nBandCount );
  pswops->panSrcBands[0] = 1;
  pswops->panDstBands = ( int * ) CPLMalloc( sizeof( int ) * pswops->nBandCount );
  pswops->panDstBands[0] = 1;

  //Optional: Use GDALTermProgress bar for console applications
  if ( progressBar )
    pswops->pfnProgress = GDALTermProgress;

  //Set transformers:
  //GenImgProjTransformer does a three stage geo-transformation:
  //1. Source pixel/line coordinates to source image georeferenced coordinates using GeoTransformation
  //2. Change projections from source coordinate system to destination coordinate system
  //3. Convert from destination image georeferenced coordinates to destination image coordinates
  pswops->pTransformerArg = GDALCreateGenImgProjTransformer( srcDataset, GDALGetProjectionRef( srcDataset ), dstDataset, GDALGetProjectionRef( dstDataset ), FALSE, 0.0, 1 );
  pswops->pfnTransformer = GDALGenImgProjTransform;

  return pswops;
}

void QgsProjectionManager::warpDatasets( GDALWarpOptions *pswops, GDALDataset *dstDataset )
{
  GDALWarpOperation warpOp;
  CPLErr err;
  warpOp.Initialize( pswops );
  err = warpOp.ChunkAndWarpImage( 0, 0, GDALGetRasterXSize( dstDataset ), GDALGetRasterYSize( dstDataset ) );
  if ( err != CE_None )
  {
    QgsLogger::debug( "QgsProjectionManager::projectGCP() failed to warp datasets." );
  }
}


void QgsProjectionManager::destroyWarpData( GDALWarpOptions *pswops )
{
  GDALDestroyGenImgProjTransformer( pswops->pTransformerArg );
  GDALDestroyWarpOptions( pswops );
}


bool QgsProjectionManager::checkProjectionInformation( QgsRasterDataset *imageDataset )
{
  GDALDataset *dataset = imageDataset->gdalDataset();
  if ( !dataset )
  {
    QgsLogger::debug( "Invalid dataset passed to QgsProjectionManager::checkProjectionInformation(GDALDataset*)." );
    return false;
  }

  const char *imageProjInfo = dataset->GetProjectionRef();
  if ( 0 == strcmp( "", imageProjInfo ) )
  {
    return false;
  }
  return true;

}


bool QgsProjectionManager::setProjectionInformation( QgsRasterDataset *imageDataset, QString projectionDefinition )
{
  GDALDataset *dataset = imageDataset->gdalDataset();
  QgsCoordinateReferenceSystem *crs = new QgsCoordinateReferenceSystem();
  crs->createFromProj4( projectionDefinition );

  if ( dataset->SetProjection( crs->toWkt().toLatin1().data() ) != CE_None )
  {
    QgsLogger::debug( "QgsProjectionManager::SetProjection failed to set projection to dataset provided." );
    return false;
  }
  return true;
}

long QgsProjectionManager::getProjectionSrsID( QgsRasterDataset *imageDataset )
{
  QgsLogger::debug( "getProjectionSrsID" );
  GDALDataset *dataset = imageDataset->gdalDataset();
  QgsCoordinateReferenceSystem *crs = new QgsCoordinateReferenceSystem();
  QString projDefinition = dataset->GetProjectionRef();
  if ( crs->createFromWkt( projDefinition ) )
  {
    QgsLogger::debug( "Succeed: getProjectionSrsID" );
    return crs->srsid();
  }
  else
  {
    QgsLogger::debug( "Fail: getProjectionSrsID" );
    return 0;
  }
}

QString QgsProjectionManager::getProjectionAuthID( QgsRasterDataset *imageDataset )
{
  GDALDataset *dataset = imageDataset->gdalDataset();
  QgsCoordinateReferenceSystem *crs = new QgsCoordinateReferenceSystem();
  QString projDefinition = dataset->GetProjectionRef();
  if ( crs->createFromWkt( projDefinition ) )
  {
    return crs->authid();
  }
  else
  {
    return "Not Found";
  }
}

long QgsProjectionManager::getProjectionEpsg( QgsRasterDataset *imageDataset )
{
  QgsLogger::debug( "getProjectionEpsg" );
  GDALDataset *dataset = imageDataset->gdalDataset();
  QgsCoordinateReferenceSystem *crs = new QgsCoordinateReferenceSystem();
  QString projDefinition = dataset->GetProjectionRef();
  if ( crs->createFromWkt( projDefinition ) )
  {
    QgsLogger::debug( "Succeed: getProjectionEpsg" );
    return crs->epsg();
  }
  else
  {
    QgsLogger::debug( "Failed: getProjectionEpsg" );
    return 0;
  }
}

bool QgsProjectionManager::transformCoordinate( double* xCoords, double* yCoords, int count, const QString& destCrs, const QString& srcCrs )
{
  OGRSpatialReference ogrSrc, ogrDest;
  OGRCoordinateTransformation *transform;

  char inputStr[500];
  char* pInput;

  strcpy( inputStr, srcCrs.toLatin1().data() );
  pInput = inputStr;
  ogrSrc.importFromWkt( &pInput );

  strcpy( inputStr, destCrs.toLatin1().data() );
  pInput = inputStr;
  ogrDest.importFromWkt( &pInput );
  transform = OGRCreateCoordinateTransformation( &ogrSrc, &ogrDest );
  if ( transform && transform->Transform( count, xCoords, yCoords ) )
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool QgsProjectionManager::crsEquals( const QString& firstCrs, const QString& secondCrs )
{
  OGRSpatialReference ogrSrc, ogrDest;
  //OGRCoordinateTransformation *transform;

  char* inputStr = new char[std::max( firstCrs.size(), secondCrs.size() )];
  char* pInput;

  strcpy( inputStr, firstCrs.toLatin1().data() );
  pInput = inputStr;
  ogrSrc.importFromWkt( &pInput );

  strcpy( inputStr, secondCrs.toLatin1().data() );
  pInput = inputStr;
  ogrDest.importFromWkt( &pInput );

  delete []inputStr;
  return ( ogrSrc.IsSame( &ogrDest ) );
}

