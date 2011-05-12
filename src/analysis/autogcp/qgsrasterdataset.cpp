/***************************************************************************
qgsrasterdataset.cpp - General purpose raster dataset and reader that provides
 access to the metadata of the image.
--------------------------------------
Date : 07-May-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsrasterdataset.cpp 606 2010-10-29 09:13:39Z jamesm $ */
#include "qgsrasterdataset.h"
#include "qgslogger.h"
#include <iostream>
#include <cstdlib>
#include <QFile>
#include <QTextStream>
#include <QByteArray>
#include <cfloat>
#include <gdalwarper.h>
#include <QDir>
#include <QTemporaryFile>
#include <algorithm>
#include <gdal.h>
#ifndef FLT_EQUALS
//Fuzzy comparison of floating point values
#define FLT_EQUALS(x,y) \
  (fabs(x-y)<FLT_EPSILON)
#endif

double openingProgressStatus = 0;
int progressFunction( double dfComplete, const char *pszMessage, void *pData )
{
  openingProgressStatus = dfComplete;
  return 1;
}

double QgsRasterDataset::progress() const
{
  return openingProgressStatus;
}

QgsRasterDataset::QgsRasterDataset( QString path, Access access, bool automaticallyOpen ):
    mBuffers( NULL ),
    mDataset( NULL ),
    mBaseDataset( NULL ),
    mVirtual( false ),
    mAccess( access ),
    mOpen( false )
{
  if ( automaticallyOpen )
  {
    if ( !open( path, access ) )
    {
      mFail = true;
    }
    else
    {
      mFail = false;
    }
  }
}

QgsRasterDataset::QgsRasterDataset( GDALDataset* dataset ):
    mFilePath( "" ),
    mBaseDataset( NULL ),
    mVirtual( false ),
    mOpen( false )
{
  if ( !dataset )
  {
    mFail  = true;
    QgsLogger::debug( "Invalid GDAL dataset. Failed to initialize QgsRasterDataset" );
  }
  else
  {
    mDataset = dataset;
    mOpen = true;
    initialize();
    mFail = false;
  }
}


QgsRasterDataset::~QgsRasterDataset()
{
  if ( isOpen() )
  {
    close();
  }
  if ( isVirtual() )
  {//Virtual raster uses a temporary north-up file
    QFile::remove( mTempPath );
  }
}

bool QgsRasterDataset::open( QString path, Access access )
{
  registerGdalDrivers();
  mFilePath = path;
  //const char* filename = QFile::encodeName(mFilePath).constData();
  /*char filename[260];
  sprintf(filename, "%s",mFilePath.toLatin1().data());*/
  char filename[FILENAME_MAX];
  strcpy( filename, QFile::encodeName( mFilePath ).data() );

  GDALAccess gAccess = GA_ReadOnly;
  switch ( access )
  {
    case ReadOnly:
      gAccess = GA_ReadOnly;
      break;
    case Update:
      gAccess = GA_Update;
  }
  GDALDatasetH handle =  GDALOpen( filename, gAccess );

  if ( handle )
  {
    if ( mOpen )
    {
      close();
    }
    mBaseDataset = ( GDALDataset* ) handle;
    double geoTransform[6];
    CPLErr result = GDALGetGeoTransform( mBaseDataset,  geoTransform );
    if (( result == CE_None
          && ( geoTransform[1] < 0.0
               || geoTransform[2] != 0.0
               || geoTransform[4] != 0.0
               || geoTransform[5] > 0.0 ) )
        || GDALGetGCPCount( mBaseDataset ) > 0 )
    {
      //Must create a virtual north-up raster
      GDALDataset* vrtDataset = ( GDALDataset* )GDALAutoCreateWarpedVRT( mBaseDataset, NULL, NULL, GRA_Bilinear, 0.1, NULL );


      if ( vrtDataset == NULL )
      {
        QgsLogger::warning( "Failed to create north-up VRT dataset for raw dataset" );
        mDataset = mBaseDataset;
        // GDALReferenceDataset(mDataset);
      }
      //Create Temporary Copy of VRT for Performance

      mTempPath = QDir::tempPath();
      if ( !mTempPath.endsWith( QDir::separator() ) )
      {
        mTempPath.append( QDir::separator() );
      }
      QFileInfo fInfo( path );
      mTempPath.append( fInfo.fileName() );

      if ( QFile::exists( mTempPath ) )
      {
        QFile::remove( mTempPath );
      }
      char tempFileName[FILENAME_MAX];
      strcpy( tempFileName, QFile::encodeName( mTempPath ).data() );
      GDALDriver* driver = mBaseDataset->GetDriver();

      //int nXSize = vrtDataset->GetRasterXSize();
      //int nYSize = vrtDataset->GetRasterYSize();
      //int nBands = vrtDataset->GetRasterCount();
      //GDALDataType eType = vrtDataset->GetRasterBand( 1 )->GetRasterDataType();
      QgsLogger::debug( "Copying Virtual Raster to temporary file..." );
      mDataset = driver->CreateCopy( tempFileName, vrtDataset, false, NULL, (GDALProgressFunc) progressFunction, NULL );
      if ( mDataset )
      {
        GDALClose( vrtDataset );
        GDALClose( mDataset );//Save to disk
        mDataset = ( GDALDataset* )GDALOpen( tempFileName, gAccess );//reopen
        mVirtual = true;
      }
      else
      {
        mDataset = vrtDataset;
      }

      if ( !mDataset )
      {
        mFail = true;
      }


    }
    else
    {
      mDataset = mBaseDataset;
      //GDALReferenceDataset(mDataset);
    }

    initialize();
    mOpen = true;
    mFail = false;
    return true;
  }
  else
  {
    QgsLogger::debug( QString( "Failed to open Image dataset. File path=" ) + mFilePath );
    return false;
  }
}

void QgsRasterDataset::close()
{
  if ( isOpen() )
  {
    if ( mDataset )
    {
      GDALClose( mDataset );
      mDataset = NULL;
    }
    if ( mBuffers )
    {
      delete [] mBuffers;
      mBuffers = NULL;
    }
    mOpen = false;
  }
}
bool QgsRasterDataset::isOpen() const
{
  return mOpen;
}

bool QgsRasterDataset::reopen()
{
  QString* path = NULL;
  if ( isVirtual() )
  {
    path = &mTempPath;
  }
  else
  {
    path = &mFilePath;
  }
  if ( QFile::exists( *path ) )
  {
    return open( *path, mAccess );
  } //End If File Exists
  else
  {
    return false; //No changes made
  }
}

void QgsRasterDataset::initialize()
{
  mXSize = mDataset->GetRasterXSize();
  mYSize = mDataset->GetRasterYSize();
  //Create Buffers
  int bandCount = mDataset->GetRasterCount();
  mBuffers = new DataBuffer<mBufferCount>[bandCount];
}

void QgsRasterDataset::geoTopLeft( double& X, double& Y ) const
{
  double coeff[6];
  this->geoCoefficients( coeff );
  X = coeff[0];
  Y = coeff[3];
}

void QgsRasterDataset::geoBottomRight( double& X, double& Y ) const
{
  double coeff[6];
  this->geoCoefficients( coeff );
  X = coeff[0] + mXSize * coeff[1] + mYSize * coeff[2];
  Y = coeff[3] + mXSize * coeff[4] + mYSize * coeff[5];
}


GDALDataset* QgsRasterDataset::gdalDataset()
{
  return mDataset;
}


void QgsRasterDataset::registerGdalDrivers()
{
  if ( 0 == GDALGetDriverCount() )
  {
    GDALAllRegister();
  }
}//void QgsRasterDataset::registerGdalDrivers()

bool QgsRasterDataset::failed() const
{
  return mFail;
}

//Wrapper for failed()
bool QgsRasterDataset::operator!() const
{
  return failed();
}

QgsRasterDataset::operator bool() const
{
  return !failed();
}

void QgsRasterDataset::imageSize( int& xSize, int& ySize ) const
{
  if ( !failed() )
  {
    xSize = mXSize;
    ySize = mYSize;
  }
}//bool QgsRasterDataset::operator!() const

int QgsRasterDataset::imageXSize() const
{
  return mXSize;
}
int QgsRasterDataset::imageYSize() const
{
  return mYSize;
}
bool QgsRasterDataset::operator==( QgsRasterDataset& other )
{
  if ( other.imageXSize() != imageXSize() || other.imageYSize() != imageYSize() || other.rasterBands() != rasterBands() )
  {
    return false;
  }
  for ( int band = 0; band < rasterBands(); band++ )
  {
    for ( int i = 0; i < imageYSize(); ++i )
    {
      for ( int j = 0; j < imageXSize(); ++j )
      {
        double me, comp;
        me = readValue( band, i, j );
        comp = other.readValue( band, i, j );
        if ( !FLT_EQUALS( me, comp ) )
        {
          return false;
        }
      }
    }
  }
  return true;
}
void* QgsRasterDataset::readLine( int theBand, int line )
{
  char message[100];
//printf("READING BAND %d, line %d\n",theBand, line);
  if ( theBand > mDataset->GetRasterCount() || theBand < 1 )
  {
    //Log debug message
    return 0;
  }

  DataBuffer<mBufferCount>& buffer = mBuffers[theBand - 1];
  for ( int i = 0; i < mBufferCount; i++ )
  {


    if ( line == buffer.line[i] && buffer.data[i] )
    {//Has this line in the buffer already
      sprintf( message, "In buffer: %d %d", line, i );
      // QgsLogger::debug(message,1,"QgsRasterDataset::readLine()");
      return buffer.data[i];
    }
  }
  //ELSE**************************************************
//QgsLogger::debug(QString("Not Cached"));
  GDALRasterBand* band = mDataset->GetRasterBand( theBand );
  GDALDataType type = band->GetRasterDataType();
  //printf("%p \n", band);
  int blockX, blockY;
  if ( line >= mYSize )
  {
    return 0;
  }

  band->GetBlockSize( &blockX, &blockY );
  // QgsLogger::debug(QString("is good "));
  //int blocksToRead = mXSize / blockX; //Usually just one.
  //int yBlock = line / blockY; //Which block will be read
  // int toAlloc = blocksToRead * blockX * blockY * GDALGetDataTypeSize(band->GetRasterDataType());
  int toAlloc = 1 * mXSize * ( GDALGetDataTypeSize( type ) / 8 );
  int curbuf = buffer.currentBuffer;
  //QgsLogger::debug(QString("TO Alloc:")+QString::number(toAlloc));

  if ( !buffer.data[curbuf] )
  {
    buffer.data[curbuf] = VSIMalloc( toAlloc );
  }
  //QgsLogger::debug("Allocated Data",1,"QgsRasterDataset::readLine()");
  //void* start;
  //CPLErr result;
  /* for (int i = 0; i < blocksToRead; ++i)
   {
     start = dataIndex(buffer.data[curbuf], theBand, i * blockX); //Where to start reading data into
     result = band->ReadBlock(i,yBlock,start);
   }*/
  band->RasterIO( GF_Read, 0, line, mXSize, 1, buffer.data[curbuf], mXSize, 1, type, 0, 0 );

// QgsLogger::debug("Not in buffer");


  buffer.currentBuffer = ( curbuf  + 1 ) % mBufferCount;
  buffer.line[curbuf]  = line;

  return buffer.data[curbuf];
}//void* QgsRasterDataset::readLine(int iband, int line)


/*WARNING does NO bounds checking.
Gets a pointer to the index'th element in the array pointed to by data.
*/
void* QgsRasterDataset::dataIndex( void* data, int band, int index )
{
  GDALDataType type = dataType( band );
  switch ( type )
  {
    case GDT_Byte:
      return ( void* ) &(( GByte * )data )[index];
      break;
    case GDT_UInt16:
      return ( void* ) &(( GUInt16 * )data )[index];
      break;
    case GDT_Int16:
      return ( void* ) &(( GInt16 * )data )[index];
      break;
    case GDT_UInt32:
      return ( void* ) &(( GUInt32 * )data )[index];
      break;
    case GDT_Int32:
      return ( void* ) &(( GInt32 * )data )[index];
      break;
    case GDT_Float32:
      return ( void* ) &(( float * )data )[index];
      break;
    case GDT_Float64:
      //  val = (( double * )data )[index];
      return ( void* ) &(( double * )data )[index];
      break;
    default:;
      //   QgsLogger::warning( "GDAL data type is not supported" );
  }
  return NULL;
}//void* QgsWaveletTransform::dataIndex(void* data, int index)

int QgsRasterDataset::rasterBands() const
{
  if ( mDataset )
  {
    int bands = mDataset->GetRasterCount();
    return bands;
  }
  else
  {
    return 0;
  }
}

double QgsRasterDataset::readValue( int band, int xPixel, int yLine, bool* succeeded )
{
  if ( xPixel < 0 ||
       xPixel >= mXSize ||
       yLine < 0 ||
       yLine >= mYSize ||
       band < 1 ||
       band > rasterBands()
     ) //Bounds check
  {
    if ( succeeded )
    {
      *succeeded = false;
    }
    return 0.0;
  }
  double value;
  void* data = readLine( band, yLine );
// char message[100];
// sprintf(message, "Data: %p", data);
// QgsLogger::debug(message,1,"QgsRasterDataset::readValue()");

  GDALDataType type = dataType( band );
  //printf("Data TYPE: %s\n", GDALGetDataTypeName(type));
  switch ( type )
  {
    case GDT_Byte:
      value = ( double )(( GByte * )data )[xPixel];
      break;
    case GDT_UInt16:
      value = ( double )(( GUInt16 * )data )[xPixel];
      break;
    case GDT_Int16:
      value = ( double )(( GInt16 * )data )[xPixel];
      break;
    case GDT_UInt32:
      value = ( double )(( GUInt32 * )data )[xPixel];
      break;
    case GDT_Int32:
      value = ( double )(( GInt32 * )data )[xPixel];
      break;
    case GDT_Float32:
      value = ( double )(( float * )data )[xPixel];
      break;
    case GDT_Float64:
      //  val = (( double * )data )[index];
      value = ( double )(( double * )data )[xPixel];
      break;
    default: return 0.0;
      //   QgsLogger::warning( "GDAL data type is not supported" );
  }//switch(type)
// QgsLogger::debug(QString::number(imageXSize()));
  return value;
}
bool QgsRasterDataset::transformCoordinate( int& pixelX, int& pixelY, double& geoX, double& geoY, int mode ) const
{
  double coef[6];
  if ( mDataset->GetGeoTransform( coef ) == CE_Failure )
  {
    return false;
  }
  if ( mode == TM_PIXELGEO )
  {
    GDALApplyGeoTransform( coef, ( double ) pixelX, ( double ) pixelY, &geoX, &geoY );
  }
  else  //TM_GEOPIXEL
  {
    double in[6];
    for ( int i = 0; i < 6; i++ )
    {
      in[i] = coef[i];
    }
    GDALInvGeoTransform( in, coef );
    double resX, resY;
    GDALApplyGeoTransform( coef,  geoX, geoY, &resX, &resY );
    pixelX = ( int ) QgsRasterDataset::round( resX );
    pixelY = ( int ) QgsRasterDataset::round( resY );
  }
  return true;
}
void QgsRasterDataset::setImageSize( int xSize, int ySize )
{
  mXSize = xSize;
  mYSize = ySize;
}
GDALDataType QgsRasterDataset::rasterDataType() const
{
  return dataType( 1 );
}
GDALDataType QgsRasterDataset::dataType( int theBand ) const
{
  if ( theBand > 0 && theBand <= rasterBands() )
  {
    return mDataset->GetRasterBand( theBand )->GetRasterDataType();
  }
  else
  {
    return GDT_Unknown;
  }

}

QString QgsRasterDataset::filePath() const
{
  return mFilePath;
}

bool QgsRasterDataset::isVirtual() const
{
  return mVirtual;
}

bool QgsRasterDataset::georeferenced() const
{
  double coeff[6];
  CPLErr result = mDataset->GetGeoTransform( coeff );
  return ( result == CE_None );
}
void QgsRasterDataset::setGeoCoefficients( double topLeftX, double pixelWidthX, double pixelWidthY, double topLeftY, double pixelHeightX, double pixelHeightY )
{
  double coeff[6];

  coeff[0] = topLeftX;
  coeff[1] = pixelWidthX;
  coeff[2] = pixelWidthY;
  coeff[3] = topLeftY;
  coeff[4] = pixelHeightX;
  coeff[5] = pixelHeightY;
  setGeoCoefficients( coeff );
}
void QgsRasterDataset::setGeoCoefficients( double* coeff )
{
  mDataset->SetGeoTransform( coeff );
}
bool QgsRasterDataset::geoCoefficients( double* coeff ) const
{
  CPLErr result = mDataset->GetGeoTransform( coeff );
  return ( result == CE_None );
}
bool QgsRasterDataset::loadWorldFile( QString filename )
{
  QFile worldFile( filename );
  if ( worldFile.exists() )
  {
    worldFile.open( QFile::ReadOnly );
    QTextStream in( &worldFile );
    double coeff[6];
    int counter = 0;
    while ( !in.atEnd() && counter < 6 )
    {
      in >> coeff[counter++];
    }
    if ( counter < 6 )
    {
      worldFile.close();
      return false;
    }

    //Map from ESRI to gdal format
    double mCoeff[6];
    mCoeff[0] = coeff[5];
    mCoeff[3] = coeff[6];
    mCoeff[1] = coeff[0];
    mCoeff[5] = coeff[3];
    mCoeff[2] = coeff[1];
    mCoeff[4] = coeff[2];
    CPLErr result = mDataset->SetGeoTransform( mCoeff );
    worldFile.close();
    return ( result == CE_None );

  }
  else
  {
    return false;
  }
}

bool QgsRasterDataset::setProjection( QString projString )
{
  return mDataset->SetProjection( projString.toLatin1().data() );
}
QString QgsRasterDataset::projection() const
{
  return QString( mDataset->GetProjectionRef() );
}

//Copies all data from one DS to another
bool QgsRasterDataset::copyDataset( GDALDataset* srcDs, GDALDataset* destDs )
{
  int xSize = std::min( srcDs->GetRasterXSize(), destDs->GetRasterXSize() );
  int ySize = std::min( srcDs->GetRasterYSize(), destDs->GetRasterYSize() );
  int bands = std::min( srcDs->GetRasterCount(), destDs->GetRasterCount() );
  GDALDataType destType = destDs->GetRasterBand( 1 )->GetRasterDataType();
  size_t nBufferSize = xSize * ( GDALGetDataTypeSize( destType ) / 8 );
  void* pBuffer = VSIMalloc( nBufferSize );
  if ( !pBuffer )
  {
    return false;
  }
  for ( int i = 1; i <= bands; ++i )
  {
    GDALRasterBand* srcBand = srcDs->GetRasterBand( i );
    GDALRasterBand* destBand = destDs->GetRasterBand( i );
    for ( int line = 0; line < ySize; ++line )
    {
      if ( CE_None == srcBand->RasterIO( GF_Read, 0, line, xSize, 1, pBuffer, xSize, 1, destType, 0, 0 ) )
      {
        destBand->RasterIO( GF_Write, 0, line, xSize, 1, pBuffer, xSize, 1, destType, 0, 0 );
      }
    }
  }
  double geoTransform[6];
  srcDs->GetGeoTransform( geoTransform );
  destDs->SetGeoTransform( geoTransform );
  destDs->SetProjection( srcDs->GetProjectionRef() );
  return true;
}
