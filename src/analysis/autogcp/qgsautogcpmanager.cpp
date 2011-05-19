/***************************************************************************
qgsmanager.cpp - This class manages the operations that need to be performed by
 the plug-in interface. Due to the clear separation of the core analysis
 classes and the plugin-specific user interface classes, a bridge is required
 between these different libraries. This class acts as the bridge, and removes
 any business logic from the presentational duties of the UI classes. This is
 done by delegating more specific operations to dedicated classes in the core
 package.
--------------------------------------
Date : 07-May-2010
Copyright : (C) 2010 by FoxHat Solutions
Author : Christoph Stallmann
Contributed : James Meyer, Francois Maass
Email : foxhat.solutions@gmail.com
***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsautogcpmanager.cpp 606 2010-10-29 09:13:39Z jamesm $ */
#include "qgsautogcpmanager.h"
#include "qgsimageanalyzer.h"
#include "qgslogger.h"
#include "qgsrpcmodel.h"
#include "qgsbilinearsampler.h"s
#include "qgsrasterlayer.h"
#include "qgstpstransform.h"
#include "qgsgeoreferencer.h"
#include <QMutex>
#define DEFAULT_DRIVER "GTiff"
QgsAutoGCPManager::QgsAutoGCPManager():
    mRawImage( NULL ),
    mRefImage( NULL ),
    mOrtho( NULL ),
    mGcpSet( NULL ),
    mProjManager( NULL ),
    mNumGcps( 60 ),
    mGeoReferenced( false ),
    mLastError( ErrorNone ),
    mOutputDriver( DEFAULT_DRIVER ),
    mChipWidth( DEFAULT_CHIP_SIZE ),
    mChipHeight( DEFAULT_CHIP_SIZE ),
    mExtractionBand( 1 ),
    mExecThread( NULL ),
    mTransformFunc( ThinPlateSpline ),
    mExecStatus( StatusDone ),
    mProgressArg( NULL ),
    mRpcThreshold( 0.85 ),
    mCorrelationThreshold( 0 )
{
  QgsLogger::debug( "Getting file filter for Raster images" );
  QgsRasterLayer::buildSupportedRasterFileFilter( mRasterFileFilter );
}
QgsAutoGCPManager::QgsAutoGCPManager( const QgsAutoGCPManager& other ):
    mRawImage( NULL ),
    mRefImage( NULL ),
    mOrtho( NULL ),
    mGcpSet( NULL ),
    mProjManager( NULL ),
    mNumGcps( 60 ),
    mGeoReferenced( false ),
    mLastError( ErrorNone ),
    mOutputDriver( DEFAULT_DRIVER ),
    mChipWidth( DEFAULT_CHIP_SIZE ),
    mChipHeight( DEFAULT_CHIP_SIZE ),
    mExtractionBand( 1 ),
    mExecThread( NULL ),
    mTransformFunc( ThinPlateSpline ),
    mExecStatus( StatusDone ),
    mProgressArg( NULL ),
    mRpcThreshold( 0.5 )
{

}

QgsAutoGCPManager::~QgsAutoGCPManager()
{
  if ( mRawImage )
  {
    delete mRawImage;
  }
  if ( mRefImage )
  {
    delete mRefImage;
  }

  if ( mGcpSet )
  {
    delete mGcpSet;
  }
  if ( mOrtho )
  {
    delete mOrtho;
  }
  if ( mExecThread )
  {
    delete mExecThread;
  }

}

bool QgsAutoGCPManager::openSensedImage()
{
  if ( mRawImage )
  {
    delete mRawImage;
  }
  mRawImage = new QgsRasterDataset( rawPath, QgsRasterDataset::ReadOnly, false );
  mProgressArg = mRawImage;
  mRawImage->open( rawPath, QgsRasterDataset::ReadOnly );
  mMutex.lock();
  mProgressArg = NULL;
  mMutex.unlock();
  if ( !QFile::exists( rawPath ) )
  {
    mLastError = ErrorFileNotFound;
    return false;
  }
  if ( mRawImage->failed() )
  {
    mLastError = ErrorOpenFailed;
    return false;
  }
  setGeoReferenced( false );
  if ( !QgsProjectionManager::checkProjectionInformation( mRawImage ) )
  {
	mLastError = ErrorMissingProjection;
    return false;
  }
  return true;
}

bool QgsAutoGCPManager::openReferenceImage()
{
  QgsLogger::debug( "Opening Reference Image " + refPath );
  QgsLogger::debug( "Encoded path: " + QFile::encodeName( refPath ) );
  if ( !QFile::exists( refPath ) )
  {
    mLastError = ErrorFileNotFound;
    return false;
  }
  if ( mRefImage )
  {
    delete mRefImage;
  }
  mRefImage = new QgsRasterDataset( refPath, QgsRasterDataset::ReadOnly, false );
  mProgressArg = mRefImage;
  mRefImage->open( refPath, QgsRasterDataset::ReadOnly );
  mMutex.lock();
  mProgressArg = NULL;
  mMutex.unlock();
  if ( mRefImage->failed() )
  {
    mLastError = ErrorOpenFailed;
    return false;
  }
  setGeoReferenced( false );

  if ( !QgsProjectionManager::checkProjectionInformation( mRefImage ) )
  {
    mLastError = ErrorMissingProjection;
    return false;
  }
  return true;
}

QgsAutoGCPManager::IOError QgsAutoGCPManager::lastError() const
{
  return mLastError;
}

void QgsAutoGCPManager::setDestinationPath( QString path )
{
  mDestPath = path;
}
QString QgsAutoGCPManager::destinationPath()const
{
  return mDestPath;
}
void QgsAutoGCPManager::setGcpCount( int count )
{
  mNumGcps = count;
}
void QgsAutoGCPManager::clearGcpSet()
{
  if ( mGcpSet )
    mGcpSet->clear();
}
QgsGcpSet* QgsAutoGCPManager::extractControlPoints()
{
  if ( extract() )
  {
    return mGcpSet;
  }
  else
  {
    return NULL;
  }
}

QgsGcpSet* QgsAutoGCPManager::matchControlPoints()
{
  if ( crossReference() )
  {
    return mGcpSet;
  }
  else
  {
    return NULL;
  }
}


bool QgsAutoGCPManager::createOrthoImage()
{
  if ( isGeoreferenced() && mRawImage && mGcpSet )
  {
    if ( !mRawImage->reopen() )
    {
      QgsLogger::debug( "Failed to reopen dataset" );
      return false;
    }
    QgsGcpSet *newGcpSet = new QgsGcpSet( *mGcpSet );
    //QgsElevationModel *e = new QgsElevationModel("/home/cstallmann/Downloads/S33E018.hgt");
    QgsRpcModel model( new QgsGcpSet( *newGcpSet ), mRawImage/*, e*/ );

    model.setRmsErrorThreshold( mRpcThreshold );
    //model.constructModel();
    while ( !model.constructModel() && newGcpSet->size() > 0 )
    {
      //If the RPC model cannot be created with the current GCP set, the last GCP is removed and we try again
      QgsLogger::debug( "Failed to construct RPC Model, removing last GCP..." );
      newGcpSet->list().removeLast();
      model.setGcpSet( new QgsGcpSet( *newGcpSet ) );
      if ( newGcpSet->size() == 0 )
      {
        QgsLogger::debug( "Failed to construct RPC Model, no GCPs left" );
        return false;
      }
    }
    QgsLogger::debug( "RPC Model RMSE = " + QString::number( model.rmsError() ) );
    QgsLogger::debug( "Suitable GCPs = " + QString::number( mGcpSet->size() ) );
    QgsBilinearSampler sampler;
    QgsOrthorectificationFilter filter( mRawImage, &model, &sampler, NULL );
    //QgsElevationModel *m = new QgsElevationModel( "/home/cstallmann/Raw_Spot24_UTM34/dem.hgt" );
    //QgsOrthorectificationFilter filter( mRawImage, &model, &sampler, m );

    filter.setDriver( outputDriver() );
    filter.setDestinationFile( destinationPath() );

    mProgressArg = &filter;
    mOrtho = filter.applyFilter();
    mMutex.lock();
    mProgressArg = NULL;
    mMutex.unlock();
    mRawImage->close();
    if ( mOrtho )
    {
      delete mOrtho;
      mOrtho = NULL;
      return true;
    }
    else
    {
      QgsLogger::debug( "Orthorectification could not be started" );
      return false;
    }
  }
  else
  {
    QgsLogger::debug( "The current set of GCPs have not been cross-referenced." );

    return false;

  }
}


void QgsAutoGCPManager::executeOperation( ExecutionFunc operation, QgsRasterDataset* pDs )
{
  if ( status() == StatusBusy )
  {
    QgsLogger::debug( "Processing is currently in progress" );
    return;
  }
  if ( mExecThread )
  {
    if ( mExecThread->isFinished() )
    {
      delete mExecThread;
    }
    else
    {
      QgsLogger::debug( "Execution thread is already running" );
      return;
    }
  }
  mExecThread = new ExecutionThread( operation, this );
  mExecStatus = StatusBusy;
  if ( pDs != NULL )
  {
    pDs->close();
  }
  mExecThread->start();
}

void QgsAutoGCPManager::executeExtraction()
{
  executeOperation( &QgsAutoGCPManager::extract, mRefImage );
}

void QgsAutoGCPManager::executeCrossReference()
{
  executeOperation( &QgsAutoGCPManager::crossReference, mRawImage );
}

void QgsAutoGCPManager::executeOrthorectification()
{
  executeOperation( &QgsAutoGCPManager::createOrthoImage, mRawImage );
}

void QgsAutoGCPManager::executeGeoTransform()
{
  executeOperation( &QgsAutoGCPManager::geoTransform, mRawImage );
}

void QgsAutoGCPManager::executeGeoreference()
{
  executeOperation( &QgsAutoGCPManager::georeference, mRawImage );
}

void QgsAutoGCPManager::executeOpenReferenceImage( QString path )
{
  refPath = path;
  executeOperation( &QgsAutoGCPManager::openReferenceImage, NULL );
}

void QgsAutoGCPManager::executeOpenRawImage( QString path )
{
  rawPath = path;
  executeOperation( &QgsAutoGCPManager::openSensedImage, NULL );
}

QgsGcpSet* QgsAutoGCPManager::gcpSet()
{
  return mGcpSet;
}

void QgsAutoGCPManager::projectGCPs()
{
  if ( !mGcpSet )
  {
    QgsLogger::debug( "Cannot project GCPs, invalid GCP set." );
    return;
  }
  //f(!mProjManager)
  //{
  //  mProjManager = new QgsProjectionManager(mRefImage, mRawImage);
  //}
  if ( mProjManager )
  {
    std::cout << "Deleted PROJECTION MANAGER" << std::endl;
    delete mProjManager;
  }
  mProjManager = new QgsProjectionManager( mRefImage, mRawImage );

  const QList<QgsGcp*>& list = mGcpSet->list();
  QList<QgsGcp*>::const_iterator it = list.constBegin();
  for ( ; it != list.constEnd(); ++it )
  {
    mProjManager->projectGCP( *it );
  }
}

bool QgsAutoGCPManager::setProjection( QgsRasterDataset *imageDataset, QString projectionDefinition )
{
  return QgsProjectionManager::setProjectionInformation( imageDataset, projectionDefinition );

}

long QgsAutoGCPManager::getProjectionSrsID( ImageFilter imageDataset )
{
  QgsRasterDataset* pDataset = NULL;
  if ( imageDataset == QgsAutoGCPManager::Reference )
  {
    pDataset = mRefImage;

  }
  else
  {
    pDataset = mRawImage;
  }
  if ( pDataset && !pDataset->isOpen() )
  {
    pDataset->reopen();
  }
  return QgsProjectionManager::getProjectionSrsID( pDataset );


}

long QgsAutoGCPManager::getProjectionEpsg( ImageFilter imageDataset )
{
  QgsRasterDataset* pDataset = NULL;
  if ( imageDataset == QgsAutoGCPManager::Reference )
  {
    pDataset = mRefImage;
  }
  else
  {
    pDataset = mRawImage;
  }
  if ( pDataset && !pDataset->isOpen() )
  {
    pDataset->reopen();
  }
  return QgsProjectionManager::getProjectionEpsg( pDataset );
}

QString QgsAutoGCPManager::getProjectionAuthID( ImageFilter imageDataset )
{
  QgsRasterDataset* pDataset = NULL;
  if ( imageDataset == QgsAutoGCPManager::Reference )
  {
    pDataset = mRefImage;

  }
  else
  {
    pDataset = mRawImage;
  }
  if ( pDataset && !pDataset->isOpen() )
  {
    pDataset->reopen();
  }
  return QgsProjectionManager::getProjectionAuthID( pDataset );


}

bool QgsAutoGCPManager::exportGcpSet( QString path )
{
  QFile file( path + ".gcp" );
  if ( file.open( QFile::WriteOnly | QFile::Truncate ) )
  {
    QTextStream out( &file );
    QList<QgsGcp*> gcpList = mGcpSet->list();
    mRawImage->open( rawPath, QgsRasterDataset::ReadOnly );
    for ( int i = 0; i < gcpList.size(); i++ )
    {
      QgsGcp *gcp = gcpList[i];
      QString var1;
      var1.sprintf( "%.15f", gcp->refX() );
      QString var2;
      var2.sprintf( "%.15f", gcp->refY() );

      int p1;
      int p2;
      double p3 = gcp->rawX();
      double p4 = gcp->rawY();
      mRawImage->mapToPixel( p3, p4, p1, p2 );
      /*QString var3;
      var3.sprintf( "%.15f",  p1 );
      QString var4;
      var4.sprintf( "%.15f", p2 );*/

      QString r = "";
      if ( i < 10 )
      {
        r += "0";
      }
      r += QString::number( i );
      out << " G00" << r << " " << QString::number( p1 ) << " " << QString::number( p2 ) << " " << var1 << " " << var2 << "  0" << "\n";
    }
    file.close();
    mRawImage->close();
  }
  return exportGcpSet( mGcpSet, path );
}

bool QgsAutoGCPManager::exportGcpSet( QgsGcpSet *gcpSet, QString path )
{
  QFile file( path );
  if ( file.open( QFile::WriteOnly | QFile::Truncate ) )
  {
    QTextStream out( &file );
    out << "mapX\tmapY\tpixelX\tpixelY\n";
    QList<QgsGcp*> gcpList = gcpSet->list();
    for ( int i = 0; i < gcpList.size(); i++ )
    {
      QgsGcp *gcp = gcpList[i];
      QString var1;
      var1.sprintf( "%.15f", gcp->refX() );
      QString var2;
      var2.sprintf( "%.15f", gcp->refY() );
      QString var3;
      var3.sprintf( "%.15f", gcp->rawX() );
      QString var4;
      var4.sprintf( "%.15f", gcp->rawY() );
      out << var1 << "\t" << var2 << "\t" << var3 << "\t" << var4 << "\n";
    }
    file.close();
    return true;
  }
  return false;
}

bool QgsAutoGCPManager::importGcpSet( QString path )
{
  try
  {
    if ( !mRefImage-> isOpen() )
    {
      mRefImage->reopen();
    }
    QgsImageAnalyzer analyzer( mRefImage );
    QFile file( path );
    if ( file.open( QFile::ReadOnly ) )
    {
      QTextStream in( &file );
      QString line = in.readLine();
      int count = 0;
      while ( !line.isNull() )
      {
        try
        {
          count++;
          QStringList list = line.split( "\t" );
          if ( list[0].toDouble() )
          {
            if ( !mGcpSet ) //if set wasn't instantiated before
            {
              mGcpSet = new QgsGcpSet();
            }
            QgsGcp *gcp = new QgsGcp();
            gcp->setRefX( list[0].toDouble() );
            gcp->setRefY( list[1].toDouble() );
            gcp->setRawX( list[2].toDouble() );
            gcp->setRawY( list[3].toDouble() );
            if ( gcp->rawX() != 0.0 && gcp->rawY() != 0.0 )
            {
              setGeoReferenced( true );
            }
            if ( mRefImage != NULL )
            {
              int pixelX;
              int pixelY;
              double coorX = gcp->refX();
              double coorY = gcp->refY();
              if ( mRefImage->transformCoordinate( pixelX, pixelY, coorX, coorY, QgsRasterDataset::TM_GEOPIXEL ) )
              {
                QgsPoint *point = new QgsPoint( pixelX, pixelY );
                gcp->setGcpChip( analyzer.extractChip( *point, mChipWidth, mChipHeight ) );
              }
            }
            if ( !( gcp->refX() == 0 && gcp->refY() == 0 ) ) //ensures that the first line of text is not read
            {
              mGcpSet->addGcp( gcp );
            }
          }
          line = in.readLine();
        }
        catch ( QtConcurrent::Exception e )
        {
          QgsLogger::debug( "Line " + QString::number( count ) + " could not be read.", 1, "qgsautogcpmanager.cpp", "QgsAutoGCPManager::importGcpSet()" );
        }
      }
      file.close();
      return true;
    }
    return false;
  }
  catch ( QtConcurrent::Exception e )
  {
    QgsLogger::debug( "File could not be read.", 1, "qgsautogcpmanager.cpp", "QgsAutoGCPManager::importGcpSet()" );
    return false;
  }
}

void QgsAutoGCPManager::addGcp( QgsGcp *gcp )
{
  if ( !mGcpSet ) //if set wasn't instantiated before
  {
    mGcpSet = new QgsGcpSet();
  }
  QgsGcp *g = new QgsGcp();
  g->setRefX( gcp->refX() );
  g->setRefY( gcp->refY() );
  g->setRawX( gcp->rawX() );
  g->setRawY( gcp->rawY() );
  QgsImageAnalyzer analyzer( mRefImage );
  if ( mRefImage != NULL )
  {
    if ( !mRefImage-> isOpen() )
    {
      mRefImage->reopen();
    }
    int pixelX;
    int pixelY;
    double coorX = gcp->refX();
    double coorY = gcp->refY();
    if ( mRefImage->transformCoordinate( pixelX, pixelY, coorX, coorY, QgsRasterDataset::TM_GEOPIXEL ) )
    {
      QgsPoint *point = new QgsPoint( pixelX, pixelY );
      gcp->setGcpChip( analyzer.extractChip( *point, mChipWidth, mChipHeight ) );
    }
  }
  mGcpSet->addGcp( g );
  QgsLogger::debug( "GCP added to set", 1, "qgsautogcpmanager.cpp", "QgsAutoGCPManager::addGcp()" );
}

void QgsAutoGCPManager::updateRefGcp( QgsGcp *gcp )
{
  QgsGcp *tempGcp;
  const QList<QgsGcp*>& list = mGcpSet->constList();
  QList<QgsGcp*>::const_iterator it;
  for ( it = list.constBegin(); it != list.constEnd(); ++it )
  {
    tempGcp = ( *it );
    if ( tempGcp->rawX() == gcp->rawX() && tempGcp->rawY() == gcp->rawY() )
    {
      tempGcp->setRefX( gcp->refX() );
      tempGcp->setRefY( gcp->refY() );
      QgsLogger::debug( "GCP updated in set", 1, "qgsautogcpmanager.cpp", "QgsAutoGCPManager::updateGcp()" );
      break;
    }
  }
}

void QgsAutoGCPManager::updateRawGcp( QgsGcp *gcp )
{
  QgsGcp *tempGcp;
  const QList<QgsGcp*>& list = mGcpSet->constList();
  QList<QgsGcp*>::const_iterator it;
  for ( it = list.constBegin(); it != list.constEnd(); ++it )
  {
    tempGcp = ( *it );
    if ( tempGcp->refX() == gcp->refX() && tempGcp->refY() == gcp->refY() )
    {
      tempGcp->setRawX( gcp->rawX() );
      tempGcp->setRawY( gcp->rawY() );
      QgsLogger::debug( "GCP updated in set", 1, "qgsautogcpmanager.cpp", "QgsAutoGCPManager::updateGcp()" );
      break;
    }
  }
}

void QgsAutoGCPManager::removeGcp( QgsGcp *gcp )
{
  QgsGcp *tempGcp;
  const QList<QgsGcp*>& list = mGcpSet->constList();
  QList<QgsGcp*>::const_iterator it;
  for ( it = list.constBegin(); it != list.constEnd(); ++it )
  {
    tempGcp = ( *it );
    if ( tempGcp->refX() == gcp->refX() && tempGcp->refY() == gcp->refY() )
    {
      mGcpSet->removeGcp( tempGcp );
      QgsLogger::debug( "GCP removed from set", 1, "qgsautogcpmanager.cpp", "QgsAutoGCPManager::removeGcp()" );
      break;
    }
  }
}

void QgsAutoGCPManager::setChipSize( int width, int height )
{
  mChipWidth = width;
  mChipHeight = height;
}

IMAGE_INFO QgsAutoGCPManager::imageInfo( QString path )
{
  QgsLogger::debug( "Collection image information for " + path );
  QFileInfo fileInfo( path );
  IMAGE_INFO info;
  QString fileName = path.right( path.length() - path.lastIndexOf( "/" ) - 1 );
  info.pFileName = fileName.left( fileName.lastIndexOf( "." ) );
  info.pFilePath = path;
  if ( fileName.contains( "." ) )
  {
    info.pFileFormat = fileName.right( fileName.length() - fileName.lastIndexOf( "." ) - 1 ).toUpper();
  }
  else
  {
    info.pFileFormat = "Unknown";
  }
  QgsHasher hasher;
  info.pHash = hasher.getFileHash( path, QgsHasher::Md5, HASH_SIZE );
  qint64 fileSize = fileInfo.size();
  if ( fileSize < 1024 )
  {
    info.pFileSize = QString::number( fileSize ) + " B";
  }
  else if ( fileSize < 1048576 )
  {
    info.pFileSize = QString::number( fileSize / 1024 ) + " KB";
  }
  else if ( fileSize < 1073741824 )
  {
    info.pFileSize = QString::number( fileSize / 1048576 ) + " MB";
  }
  else if ( fileSize >= 1073741824 )
  {
    info.pFileSize = QString::number( fileSize / 1073741824 ) + " GB";
  }
  else
  {
    info.pFileSize = "Unknown";
  }
  QgsRasterDataset *image = NULL;
  if ( mRefImage != NULL && mRefImage->filePath() == path )
  {
    if ( !mRefImage-> isOpen() )
    {
      mRefImage->reopen();
    }
    image = mRefImage;
  }
  else if ( mRawImage != NULL && mRawImage->filePath() == path )
  {
    if ( !mRawImage-> isOpen() )
    {
      mRawImage->reopen();
    }
    image = mRawImage;
  }
  if (( image ) != NULL )
  {
    info.pRasterWidth = QString::number( image->gdalDataset()->GetRasterXSize() );
    info.pRasterHeight = QString::number( image->gdalDataset()->GetRasterYSize() );
    info.pRasterBands = QString::number( image->gdalDataset()->GetRasterCount() );
    double adfGeoTransform[6];
    if ( image->gdalDataset()->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
      QString var1;
      QString var2;
      info.pOriginCoordinates = var1.sprintf( "%.6f  %.6f", adfGeoTransform[0], adfGeoTransform[3] );
      info.pPixelSize = var2.sprintf( "%.6f  %.6f", adfGeoTransform[1], adfGeoTransform[5] );
    }
    info.pProjectionInfo = image->gdalDataset()->GetProjectionRef();
  }
  else
  {
    info.pRasterWidth = "Unknown";
    info.pRasterHeight = "Unknown";
    info.pRasterBands = "Unknown";
    info.pProjectionInfo = "Unknown";
    info.pPixelSize = "Unknown";
    info.pOriginCoordinates = "Unknown";
  }
  info.pDateCreated = fileInfo.created();
  info.pDateModified = fileInfo.lastModified();
  info.pDateRead = fileInfo.lastRead();
  QgsLogger::debug( "Image information done" );
  return info;
}

bool QgsAutoGCPManager::connectDatabase( QString path )
{
  mDataSource = new QgsDataSource( QgsDataSource::SQLite, QgsHasher::Md5 );
  return mDataSource->createDatabase( path );
}

bool QgsAutoGCPManager::connectDatabase( QString name, QString username, QString password, QString host )
{
  /*mDataSource = new QgsDataSource( QgsDataSource::Postgres, QgsHasher::Md5 );
  return mDataSource->createDatabase( name, username, password, host );*/
  return false;
}

QgsGcpSet* QgsAutoGCPManager::loadDatabase( ImageFilter filter )
{
  if ( mRefImage != NULL )
  {
    if ( !mRefImage-> isOpen() )
    {
      mRefImage->reopen();
    }
    QgsGcpSet *set = NULL;
    if ( filter == QgsAutoGCPManager::Reference )
    {
      set = mDataSource->selectByImage( mRefImage, NULL );
    }
    else if ( filter == QgsAutoGCPManager::Raw )
    {
      try
      {
        set = mDataSource->selectByImage( NULL, mRawImage );
      }
      catch ( QtConcurrent::Exception e )
      {
        QgsLogger::debug( "No GCPs found for the raw image" );
      }
    }
    else if ( filter == QgsAutoGCPManager::Both )
    {
      set = mDataSource->selectByImage( mRefImage, mRawImage );
      setGeoReferenced( true );
    }
    if ( !mGcpSet ) //if set wasn't instantiated before
    {
      mGcpSet = new QgsGcpSet();
    }
    const QList<QgsGcp*> &list = set->constList();
    QList<QgsGcp*>::const_iterator it;
    if ( /*mDataSource->type() == QgsDataSource::Postgres ||*/ mDataSource->type() == QgsDataSource::SQLite )
    {
      QgsImageAnalyzer analyzer( mRefImage );
      for ( it = list.constBegin(); it != list.constEnd(); ++it )
      {
        QgsGcp *gcp = new QgsGcp();
        gcp->setRawX(( *it )->rawX() );
        gcp->setRawY(( *it )->rawY() );
        gcp->setRefZ(( *it )->refZ() );
        gcp->setRefX(( *it )->refX() );
        gcp->setRefY(( *it )->refY() );
        if ( gcp->rawX() != 0.0 && gcp->rawY() != 0.0 )
        {
          setGeoReferenced( true );
        }
        int pixelX;
        int pixelY;
        double coorX = gcp->refX();
        double coorY = gcp->refY();
        if ( mRefImage->transformCoordinate( pixelX, pixelY, coorX, coorY, QgsRasterDataset::TM_GEOPIXEL ) )
        {
          QgsPoint *point = new QgsPoint( pixelX, pixelY );
          gcp->setGcpChip( analyzer.extractChip( *point, mChipWidth, mChipHeight ) );
        }
        mGcpSet->addGcp( gcp );
      }
    }
  }
  return mGcpSet;
}

QgsGcpSet* QgsAutoGCPManager::loadDatabaseByLocation( double pixelWidthX, double pixelWidthY, double pixelHeightX, double pixelHeightY, double topLeftX, double topLeftY, QString projection )
{
  QgsGcpSet *set = mDataSource->selectByLocation( pixelWidthX, pixelWidthY, pixelHeightX, pixelHeightY, topLeftX, topLeftY, projection );
  if ( !mGcpSet ) //if set wasn't instantiated before
  {
    mGcpSet = new QgsGcpSet();
  }
  const QList<QgsGcp*> &list = set->constList();
  QList<QgsGcp*>::const_iterator it;
  if ( /*mDataSource->type() == QgsDataSource::Postgres ||*/ mDataSource->type() == QgsDataSource::SQLite )
  {
    QgsImageAnalyzer analyzer( mRefImage );
    for ( it = list.constBegin(); it != list.constEnd(); ++it )
    {
      QgsGcp *gcp = new QgsGcp();
      gcp->setRawX(( *it )->rawX() );
      gcp->setRawY(( *it )->rawY() );
      gcp->setRefZ(( *it )->refZ() );
      gcp->setRefX(( *it )->refX() );
      gcp->setRefY(( *it )->refY() );
      if ( gcp->rawX() != 0.0 && gcp->rawY() != 0.0 )
      {
        setGeoReferenced( true );
      }
      if ( mRefImage != NULL )
      {
        if ( !mRefImage-> isOpen() )
        {
          mRefImage->reopen();
        }
        int pixelX;
        int pixelY;
        double coorX = gcp->refX();
        double coorY = gcp->refY();
        if ( mRefImage->transformCoordinate( pixelX, pixelY, coorX, coorY, QgsRasterDataset::TM_GEOPIXEL ) )
        {
          QgsPoint *point = new QgsPoint( pixelX, pixelY );
          gcp->setGcpChip( analyzer.extractChip( *point, mChipWidth, mChipHeight ) );
        }
      }
      mGcpSet->addGcp( gcp );
    }
  }
  return mGcpSet;
}

QgsGcpSet* QgsAutoGCPManager::loadDatabaseByHash( QString hash )
{
  if ( mRefImage != NULL )
  {
    if ( !mRefImage-> isOpen() )
    {
      mRefImage->reopen();
    }
    QgsGcpSet *set = mDataSource->selectByHash( hash, "" );
    if ( !mGcpSet ) //if set wasn't instantiated before
    {
      mGcpSet = new QgsGcpSet();
    }
    const QList<QgsGcp*> &list = set->constList();
    QList<QgsGcp*>::const_iterator it;
    if ( /*mDataSource->type() == QgsDataSource::Postgres ||*/ mDataSource->type() == QgsDataSource::SQLite )
    {
      QgsImageAnalyzer analyzer( mRefImage );
      for ( it = list.constBegin(); it != list.constEnd(); ++it )
      {
        QgsGcp *gcp = new QgsGcp();
        gcp->setRawX(( *it )->rawX() );
        gcp->setRawY(( *it )->rawY() );
        gcp->setRefZ(( *it )->refZ() );
        gcp->setRefX(( *it )->refX() );
        gcp->setRefY(( *it )->refY() );
        if ( gcp->rawX() != 0.0 && gcp->rawY() != 0.0 )
        {
          setGeoReferenced( true );
        }
        int pixelX;
        int pixelY;
        double coorX = gcp->refX();
        double coorY = gcp->refY();
        if ( mRefImage->transformCoordinate( pixelX, pixelY, coorX, coorY, QgsRasterDataset::TM_GEOPIXEL ) )
        {
          QgsPoint *point = new QgsPoint( pixelX, pixelY );
          gcp->setGcpChip( analyzer.extractChip( *point, mChipWidth, mChipHeight ) );
        }
        mGcpSet->addGcp( gcp );
      }
    }
  }
  return mGcpSet;
}

bool QgsAutoGCPManager::saveDatabase()
{
  return mDataSource->insertGcpSet( mGcpSet, mRefImage, mRawImage );
}

bool QgsAutoGCPManager::detectGcps( ImageFilter filter )
{
  if ( filter == QgsAutoGCPManager::Reference )
  {
    QgsGcpSet *set = mDataSource->selectByImage( mRefImage, NULL );
    if ( set->size() > 0 )
    {
      return true;
    }
  }
  else if ( filter == QgsAutoGCPManager::Raw || filter == QgsAutoGCPManager::Both )
  {
    try
    {
      if ( mRefImage != NULL )
      {
        QgsGcpSet *set = mDataSource->selectByImage( mRefImage, mRawImage );
        const QList<QgsGcp*> list = set->constList();
        for ( int i = 0; i < list.size(); i++ )
        {
          if ( list[i]->rawX() > 0 || list[i]->rawY() > 0 )
          {
            return true;
          }
        }
      }
    }
    catch ( QtConcurrent::Exception e )
    {
      QgsLogger::debug( "No GCPs found for the raw image" );
    }
  }
  return false;
}

bool QgsAutoGCPManager::isGeoreferenced() const
{
  return mGeoReferenced;
}

void QgsAutoGCPManager::setGeoReferenced( bool value )
{
  mGeoReferenced = value;
}


QList<DRIVER_INFO> QgsAutoGCPManager::readDriverSource( QString path )
{
  QList<DRIVER_INFO> list;
  if ( !path.isNull() )
  {
    sqlite3 *database;
    int rc = sqlite3_open( path.toAscii().data(), &database );
    if ( rc )
    {
      sqlite3_close( database );
    }
    else
    {
      char **res;
      int row, col;
      char *error;
      rc = sqlite3_get_table( database, "SELECT * FROM drivers", &res, &row, &col, &error );
      if ( rc == SQLITE_OK )
      {
        for ( int i = 1; i < row; ++i ) //Start at 1, to skip the headers
        {
          DRIVER_INFO info;
          info.pLongName = QString( res[i*col] );
          info.pShortName = QString( res[i*col+1] );
          info.pIssues = QString( res[i*col+2] ).replace( "\\n", "\n" );
          list.append( info );
        }
        sqlite3_free_table( res );
      }
      sqlite3_close( database );
    }
  }
  return list;
}

void QgsAutoGCPManager::setExtractionBand( int nBandNumer )
{
  if ( nBandNumer < 1 )
  {
    nBandNumer = 1;
  }
  mExtractionBand = nBandNumer;
}

double QgsAutoGCPManager::progress()
{
  double result;
  mMutex.lock();
  if ( NULL != mProgressArg )
  {
    result = mProgressArg->progress();
    if ( result > 0 && result < 1 )
    {
      setStatus( StatusBusy );
    }
  }
  else
  {
    result =  0.0;
  }
  mMutex.unlock();
  return result;
}

bool QgsAutoGCPManager::extract()
{

  QgsLogger::debug( "Starting Control Point Extraction", 1, "qgsautogcpmanager.cpp", "QgsAutoGCPManager::extractControlPoints()" );
  if ( mRefImage )
  {
    mRefImage->reopen();
    QgsImageAnalyzer* pIA = new QgsImageAnalyzer( mRefImage );

    mProgressArg = pIA;

    pIA->setChipSize( mChipWidth, mChipHeight );
    pIA->setTransformBand( mExtractionBand );
    QgsGcpSet* pSet =  pIA->extractGcps( mNumGcps );
    mRefImage->close();

    if ( pSet )
    {
      if ( mGcpSet )//If already a current set, add to it.
      {
        const QList<QgsGcp*>& list = pSet->constList();
        QList<QgsGcp*>::const_iterator it;
        for ( it = list.constBegin(); it != list.constEnd(); ++it )
        {
          mGcpSet->addGcp( *it );
        }
      }
      else  //Else make this the default set.
      {
        mGcpSet = pSet;
      }
    }
    else
    {
      QgsLogger::debug( "Failed to extract GCPs", 1, "qgsautogcpmanager.cpp", "QgsAutoGCPManager::extractControlPoints()" );

      mMutex.lock();
      mProgressArg = NULL;
      delete pIA;
      mMutex.unlock();
      return false;
    }
    mMutex.lock();
    mProgressArg = NULL;
    delete pIA;
    mMutex.unlock();

    return true;
  }
  else
  {
    QgsLogger::debug( "Invalid Reference Image", 1, "qgsautogcpmanager.cpp", "QgsAutoGCPManager::extractControlPoints()" );
    mProgressArg = NULL;
    return false;
  }
}

bool QgsAutoGCPManager::crossReference()
{
  if ( mRawImage )
  {
    mRawImage->reopen();
    QgsImageAnalyzer* pIA = new QgsImageAnalyzer( mRawImage );
    mProgressArg = pIA;
    pIA->setTransformBand( mExtractionBand );
    pIA->setCorrelationThreshold( mCorrelationThreshold );

#ifdef BRUTE_FORCE_MATCHING
    QgsGcpSet* pSet = pIA->bruteMatchGcps( mGcpSet );
    //QgsGcpSet* pSet = pIA->matchGcps( mGcpSet );
#else
    QgsGcpSet* pSet = pIA->matchGcps( mGcpSet );
#endif

    if ( pSet )
    {
      mGcpSet = pSet;
      setGeoReferenced( true );
    }
    mMutex.lock();
    mProgressArg = NULL;
    delete pIA;
    mMutex.unlock();
    mRawImage->close();
    return ( pSet != NULL );
  }
  return false;
}


bool QgsAutoGCPManager::georeference()
{
  if ( isGeoreferenced() )
  {

    mRawImage->reopen();
    QgsGeoreferencer georef;
    mProgressArg = &georef;
    georef.setDriver( outputDriver() );
    georef.setGcpSet( mGcpSet );
    georef.setOutputFile( mDestPath );
    georef.setSourceDataset( mRawImage );
    bool result =  georef.applyGeoreference();
    mMutex.lock();
    mProgressArg = NULL;
    mMutex.unlock();
    mRawImage->close();
    return result;
  }
  return false;
}

bool QgsAutoGCPManager::geoTransform()
{
  if ( !mRawImage )
  {
    QgsLogger::debug( "Raw image has not been set" );
    return false;
  }

  if ( !mGcpSet || mGcpSet->size() == 0 )
  {
    QgsLogger::debug( "Invalid or empty GCP set" );
    return false;
  }
  QgsGcpTransform* pTransform = NULL;
  switch ( mTransformFunc )
  {
    case ThinPlateSpline:
      pTransform = new QgsTpsTransform();
      break;
  }
  if ( NULL != pTransform )
  {
    mRawImage->reopen();
    mProgressArg = pTransform;
    pTransform->setDestinationFile( mDestPath );
    pTransform->setDriver( mOutputDriver );
    pTransform->setGcpSet( mGcpSet );
    pTransform->setSourceImage( mRawImage );
    bool result =  pTransform->applyTransform();

    mMutex.lock();
    mProgressArg = NULL;
    delete pTransform;
    mMutex.unlock();
    mRawImage->close();
    return result;
  }
  else
  {
    QgsLogger::debug( "No compatible transformation has been set" );
    return false;
  }
}

QgsAutoGCPManager::ExecutionThread::ExecutionThread( QgsAutoGCPManager::ExecutionFunc pfnOperation, QgsAutoGCPManager *pManager )
{
  this->mManager = pManager;
  this->mOperation = pfnOperation;
}

void QgsAutoGCPManager::ExecutionThread::run()
{
  mManager->setStatus( StatusBusy );
  if (( mManager->*mOperation )() )
  {
    mManager->setStatus( StatusDone );
  }
  else
  {
    mManager->setStatus( StatusFailed );
  }
  printf( "Thread operation complete\n" );
}

bool QgsAutoGCPManager::done() const
{
  bool isdone = ( status() != StatusBusy && ( !mExecThread || mExecThread->isFinished() ) );
  return isdone;
}

double QgsAutoGCPManager::correlationThreshold()const
{
  return mCorrelationThreshold;
}
void QgsAutoGCPManager::setCorrelationThreshold( double value )
{
  mCorrelationThreshold = value;
}
