
#include <QFile>
#include "qgsgeoreferencer.h"
#include "vrtdataset.h"
#include "qgslogger.h"
QgsGeoreferencer::QgsGeoreferencer()
    : mSrcDs( NULL )
    , mGcpSet( NULL )
    , mOutputFile( "" )
    , mDstCRS( "" )
    , mOutputType( GDT_Unknown )
    , mError( ErrorNone )
{

}

bool QgsGeoreferencer::applyGeoreference()
{
  mError = ErrorNone;
  if ( mGcpSet == NULL
       || mSrcDs == NULL
       || mOutputFile.isNull()
       || mOutputFile.size() == 0 )
  {
    QgsLogger::debug( QString().sprintf( "Georeferencer is not properly initialized. [mGcpSet=%p; mSrcDs=%p; mOutputFile='%s';",
                                         mGcpSet, mSrcDs, mOutputFile.toLatin1().data() ) );
    mError = ErrorFailed;
    return false;
  }
  if ( mGcpSet->size() == 0 )
  {
    QgsLogger::debug( "Georeference Failed. GCP Set is empty" );
    mError = ErrorNoGcps;
    return false;
  }

  GDALDataset* srcDs = mSrcDs->gdalDataset();



  /*********************************************
   * Create virtual dataset
   *********************************************/
  int nXSize = srcDs->GetRasterXSize();
  int nYSize = srcDs->GetRasterYSize();
  VRTDataset* pVDS = ( VRTDataset* )VRTCreate( nXSize, nYSize );


  /***************************
   * Get the output projection
   ***************************/
  const char* pGcpProjection = NULL;
  const char* emptyString = "";

  QByteArray byteArray;

  //Determine
  if ( !mDstCRS.isEmpty() )
  {
    byteArray = mDstCRS.toLatin1();
    pGcpProjection = byteArray.data();
  }
  else if ( NULL != srcDs->GetGCPProjection() && strlen( srcDs->GetGCPProjection() ) > 0 )
  {
    pGcpProjection = srcDs->GetGCPProjection();
  }
  else if ( NULL != srcDs->GetProjectionRef() )
  {
    pGcpProjection = srcDs->GetProjectionRef();
  }
  else
  {
    pGcpProjection = emptyString;
  }

  //QgsLogger::debug(QString().sprintf("Output CRS = %s", pGcpProjection));

  /*******************************
   * Compile list of GCP's
   *******************************/


  const QList<QgsGcp*>& gcplist = mGcpSet->constList();
  int numGcps = gcplist.size();
  GDAL_GCP* pGcps = new GDAL_GCP[numGcps];
  GDALInitGCPs( numGcps, pGcps );
  for ( int i = 0; i < gcplist.size(); ++i )
  {
    QgsGcp* gcp = gcplist.at( i );

    int pixelX, pixelY;
    double geoX = gcp->rawX(),
                  geoY = gcp->rawY();
    mSrcDs->transformCoordinate( pixelX, pixelY, geoX, geoY, QgsRasterDataset::TM_GEOPIXEL );
    pGcps[i].dfGCPPixel = pixelX;
    pGcps[i].dfGCPLine = pixelY;
    pGcps[i].dfGCPX = gcp->refX();
    pGcps[i].dfGCPY = gcp->refY();
  }

  pVDS->SetGCPs( numGcps, pGcps, pGcpProjection );
  GDALDeinitGCPs( numGcps, pGcps );
  delete [] pGcps;
  /*********************************
   * Transfer meta data
   *********************************/
  pVDS->SetMetadata( srcDs->GetMetadata() );

  /************************************
   * Procees bands
   ************************************/
  for ( int i = 0; i < srcDs->GetRasterCount(); ++i )
  {
    VRTSourcedRasterBand* pVRTBand;
    GDALRasterBand* pSrcBand = srcDs->GetRasterBand( i + 1 );
    GDALDataType eType;

    if ( mOutputType == GDT_Unknown )
    {
      eType = pSrcBand->GetRasterDataType();
    }
    else
    {
      eType = mOutputType;
    }

    /*****************************
     * Add the band
     *****************************/
    pVDS->AddBand( eType, NULL );
    pVRTBand = ( VRTSourcedRasterBand* ) pVDS->GetRasterBand( i + 1 );

    pVRTBand->AddSimpleSource( pSrcBand,
                               0, 0, nXSize, nYSize, //Source window
                               0, 0, nXSize, nYSize );//Dest window
    pVRTBand->CopyCommonInfoFrom( pSrcBand );
  }

  /***********************************
   * Create output file
   ***********************************/
  if ( QFile::exists( mOutputFile ) )
  {
    QFile::remove( mOutputFile );
  }

  GDALDriver* dstDriver = NULL;
  if ( mDriver.size() > 0 )
  {
    dstDriver = ( GDALDriver* ) GDALGetDriverByName( mDriver.toLatin1().data() );
  }
  if ( dstDriver == NULL )
  {
    dstDriver = ( GDALDriver* ) srcDs->GetDriver();
  }

  GDALDataset* pOutputDs = NULL;
  if ( dstDriver != NULL )
  {
    pOutputDs = dstDriver->CreateCopy( mOutputFile.toLatin1().data()
                                       , pVDS
                                       , false
                                       , NULL
                                       , (GDALProgressFunc) updateProgress
                                       , ( void* )this );
  }
  else
  {
    mError = ErrorDriver;
    QgsLogger::debug( "Failed to get Driver for output georeferenced image" );
  }

  if ( pOutputDs != NULL )
  {
    GDALClose(( GDALDatasetH ) pOutputDs );
  }
  else
  {
    QgsLogger::debug( "Failed to create output dataset" );
    if ( ErrorNone == mError )
    {
      mError = ErrorFailed;
    }
  }

  GDALClose(( GDALDatasetH ) pVDS );
  if ( ErrorNone == mError )
  {
    return true;
  }
  else
  {
    return false;
  }

}

double QgsGeoreferencer::progress() const
{

  return mProgress;
}

void QgsGeoreferencer::setProgress( double value )
{

  mProgress = value;
}

int QgsGeoreferencer::updateProgress( double dfComplete, const char*, void* pProgressArg )
{
  QgsGeoreferencer* pGeoref = static_cast<QgsGeoreferencer*>( pProgressArg );
  pGeoref->setProgress( dfComplete );
  return 1;
}
