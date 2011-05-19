/***************************************************************************
qgsrpcmodel.cpp - This class encapsulates a Rational Polynomial Coefficients
model, or RFM (Rational Function Model), which is constructed using GCPs, hence
it is terrain-dependent. Its two public methods return values indicating the
orthorectified normalised pixel position for each pixel in the original image.

This class was implemented following the solution derived in the article:

Tao, C.V., Hu, Y., 2001. A comprehensive study on the rational function model
for photogrammetric processing, Photogrammetry Engineering and Remote Sensing,
67(12), pp. 1347-1357
--------------------------------------
Date : 12-July-2010
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
/* $Id: qgsrpcmodel.cpp 623 2011-01-20 13:49:34Z goocreations $ */

#include "qgsrpcmodel.h"
#include "qgselevationmodel.h"
#include "qgsgcpset.h"
#include "qgsmatrix.h"
#include "qgslogger.h"
#include "qgsautogcpmanager.h"
#include <vector>

#ifndef SQUARE
#define SQUARE(x) ((x)*(x))
#endif
QgsRpcModel::QgsRpcModel( QgsGcpSet* gcpSet, QgsRasterDataset *img, QgsElevationModel* dem ):
    mInitialized( false ), mImageData( img ), mDem( dem ), mGcpSet( gcpSet ), mUseDem( dem != NULL ), mInit( NULL ), mThreshold( RMSE_THRESHOLD )
{
  mImageData->geoTopLeft( mGeoTopLeftX, mGeoTopLeftY );
  mImageData->geoBottomRight( mGeoBottomRightX, mGeoBottomRightY );
  mGeoWidth = mGeoBottomRightX - mGeoTopLeftX;
  mGeoHeight = mGeoBottomRightY - mGeoTopLeftY;
  mImageXSize = mImageData->imageXSize();
  mImageYSize = mImageData->imageYSize();

  if ( mUseDem )
  {
    mInit = &QgsRpcModel::initialize;
  }
  else
  {
    mInit = &QgsRpcModel::initializeNoDem;
  }
}

QgsRpcModel::~QgsRpcModel()
{
  while ( !mGcpSet->list().isEmpty() )
  {
    delete( mGcpSet->list().takeAt( 0 ) );
  }
  delete mGcpSet;
}

bool QgsRpcModel::constructModel()
{
  if ( !mGcpSet )
  {
    return false;
  }
  QgsLogger::debug( "Starting RPC initialization. GCP Count: " + QString::number( mGcpSet->size() ) );
  mInitialized = ( this->*mInit )();
  QgsLogger::debug( "RPC RMS Error threshold = " + QString::number( mThreshold ) );
  QgsGcp* pMaxGcp = NULL;
  double fHigestDev = 0.0;
  while ( mInitialized && calculateRmsError( &pMaxGcp, &fHigestDev ) > mThreshold
          && mGcpSet->size() > GCP_MINIMUM )
  {
    mGcpSet->removeGcp( pMaxGcp );
    mInitialized = ( this->*mInit )();
  }

  if ( !mInitialized )
  {
    QgsLogger::debug( "Failed to initialize RPC model" );
    return false;
  }
  if ( mRMSE > mThreshold )
  {
    QgsLogger::debug( "RPC Model refinement reached minimum number of required GCPs. RMSE remains above threshold" );
  }

  return mInitialized;
}
double QgsRpcModel::r( double X, double Y, double Z )
{
  if ( mInitialized && mUseDem )
  {
    //offset and scale X, Y to range [-1,1]
    X = (( X - mGeoTopLeftX ) / (( mGeoWidth ) / 2.0 ) ) - 1;
    Y = (( Y - mGeoTopLeftY ) / (( mGeoHeight ) / 2.0 ) ) - 1;

    //offset and scale Z to range [-1,1] using DEM minimum and maximum values
    if ( mDem )
      Z = (( Z - mDem->lowestElevationValue() ) / (( mDem->highestElevationValue() - mDem->lowestElevationValue() ) / 2.0 ) ) - 1;
    else Z = 0;

    //num uses J[0] to J[19]
    double num =  J[0]  + J[1] * Z      + J[2] * Y      + J[3] * X      + J[4] * Z * Y
                  + J[5] * Z * X    + J[6] * Y * X    + J[7] * Z * Z    + J[8] * Y * Y    + J[9] * X * X
                  + J[10] * Z * Y * X + J[11] * Z * Z * Y + J[12] * Z * Z * X + J[13] * Y * Y * Z + J[14] * Y * Y * X
                  + J[15] * Z * X * X + J[16] * Y * X * X + J[17] * Z * Z * Z + J[18] * Y * Y * Y + J[19] * X * X * X;

    //den uses 1 and also J[20] to J[38]
    double den =  1     + J[20] * Z     + J[21] * Y     + J[22] * X     + J[23] * Z * Y
                  + J[24] * Z * X   + J[25] * Y * X   + J[26] * Z * Z   + J[27] * Y * Y   + J[28] * X * X
                  + J[29] * Z * Y * X + J[30] * Z * Z * Y + J[31] * Z * Z * X + J[32] * Y * Y * Z + J[33] * Y * Y * X
                  + J[34] * Z * X * X + J[35] * Y * X * X + J[36] * Z * Z * Z + J[37] * Y * Y * Y + J[38] * X * X * X;

    return (( num / den ) + 1 )*(( mImageYSize ) / 2.0 );
  }
  else if ( mInitialized )
  {
    //offset and scale X, Y to range [-1,1]
    X = (( X - mGeoTopLeftX ) / (( mGeoWidth ) / 2.0 ) ) - 1;
    Y = (( Y - mGeoTopLeftY ) / (( mGeoHeight ) / 2.0 ) ) - 1;

    double num =  j[0] + j[1] * Y + j[2] * X + j[3] * Y * X + j[4] * Y * Y + j[5] * X * X;
    double den =  1 + j[6] * Y + j[7] * X + j[8] * Y * X + j[9] * Y * Y + j[10] * X * X;
    if ( FLT_ZERO( den ) )
    {
      QgsLogger::debug( "Denom is zero" );
      return ( Y + 1 ) *(( mGeoHeight ) / 2.0 );
    }
    return (( num / den ) + 1 )*(( mImageYSize ) / 2.0 );
  }
  else return -1;
}


double QgsRpcModel::c( double X, double Y, double Z )
{
  if ( mInitialized && mUseDem )
  {
    //offset and scale X, Y to range [-1,1]
    X = (( X - mGeoTopLeftX ) / (( mGeoWidth ) / 2.0 ) ) - 1;
    Y = (( Y - mGeoTopLeftY ) / (( mGeoHeight ) / 2.0 ) ) - 1;

    //offset and scale Z to range [-1,1] using DEM minimum and maximum values
    if ( mDem )
      Z = (( Z - mDem->lowestElevationValue() ) / (( mDem->highestElevationValue() - mDem->lowestElevationValue() ) / 2.0 ) ) - 1;
    else Z = 0;

    //num uses K[0] to K[19]
    double num =  K[0]  + K[1] * Z      + K[2] * Y      + K[3] * X      + K[4] * Z * Y
                  + K[5] * Z * X    + K[6] * Y * X    + K[7] * Z * Z    + K[8] * Y * Y    + K[9] * X * X
                  + K[10] * Z * Y * X + K[11] * Z * Z * Y + K[12] * Z * Z * X + K[13] * Y * Y * Z + K[14] * Y * Y * X
                  + K[15] * Z * X * X + K[16] * Y * X * X + K[17] * Z * Z * Z + K[18] * Y * Y * Y + K[19] * X * X * X;

    //den uses 1 and also K[20] to K[38]
    double den =  1     + K[20] * Z     + K[21] * Y     + K[22] * X     + K[23] * Z * Y
                  + K[24] * Z * X   + K[25] * Y * X   + K[26] * Z * Z   + K[27] * Y * Y   + K[28] * X * X
                  + K[29] * Z * Y * X + K[30] * Z * Z * Y + K[31] * Z * Z * X + K[32] * Y * Y * Z + K[33] * Y * Y * X
                  + K[34] * Z * X * X + K[35] * Y * X * X + K[36] * Z * Z * Z + K[37] * Y * Y * Y + K[38] * X * X * X;

    return (( num / den ) + 1 )*(( mImageXSize ) / 2.0 );
  }
  else if ( mInitialized )
  {
    //offset and scale X, Y to range [-1,1]
    X = (( X - mGeoTopLeftX ) / (( mGeoWidth ) / 2.0 ) ) - 1;
    Y = (( Y - mGeoTopLeftY ) / (( mGeoHeight ) / 2.0 ) ) - 1;


    double num =  k[0] + k[1] * Y + k[2] * X + k[3] * Y * X + k[4] * Y * Y + k[5] * X * X;
    double den =  1 + k[6] * Y + k[7] * X + k[8] * Y * X + k[9] * Y * Y + k[10] * X * X;
    if ( X > 1 || X < -1 || Y > 1 || Y < -1 )
    {
      //QgsLogger::debug( "Coordinates aren't properly normalized" );
      return ( X + 1 ) *(( mGeoWidth ) / 2.0 );
    }
    double returnVal = (( num / den ) + 1 ) * (( mImageXSize ) / 2.0 );
    return returnVal;
  }
  else return -1;
}

bool QgsRpcModel::initialize( )
{
  QList<QgsGcp*> list = mGcpSet->list();
  int n = list.size();

  //initialise matrices
  QgsMatrix R = QgsMatrix( n, 1 );
  QgsMatrix C = QgsMatrix( n, 1 );
  /*QgsMatrix M = QgsMatrix( n, 11);
  QgsMatrix N = QgsMatrix( n, 11);*/
  QgsMatrix M = QgsMatrix( n, 39 );
  QgsMatrix N = QgsMatrix( n, 39 );

  for ( int m = 0; m < n; m++ )
  {
    M( m, 0 ) = 1;
    N( m, 0 ) = 1;
  }

  //set values
  for ( int i = 0; i < n; i++ )
  {
    QgsGcp* gcp = list.at( i );
    //get and transform r, c to pixel values relative to raw image
    double rGeo = gcp->rawY();
    double cGeo = gcp->rawX();
    int rPixel, cPixel;
    mImageData->transformCoordinate( cPixel, rPixel,  cGeo, rGeo, QgsRasterDataset::TM_GEOPIXEL );
    //scale r, c (pixel) values to range [-1, 1]
    double r = ( rPixel / ( mImageYSize / 2.0 ) ) - 1;
    double c = ( cPixel / ( mImageXSize / 2.0 ) ) - 1;

    //get X,Y (geographical) values from GCP
    double X = gcp->refX();
    double Y = gcp->refY();
    //scale to range [-1,1] using original image dimensions
    X = (( X - mGeoTopLeftX ) / (( mGeoWidth ) / 2.0 ) ) - 1;
    Y = (( Y - mGeoTopLeftY ) / (( mGeoHeight ) / 2.0 ) ) - 1;

    double Z;
    //get Z (geographical) value from GCP, scale to range [-1,1] using DEM minimum and maximum values
    /*if ( mDem )
    {
      Z = gcp->refZ();
      Z = (( Z - mDem->lowestElevationValue() ) / (( mDem->highestElevationValue() - mDem->lowestElevationValue() ) / 2.0 ) ) - 1;
    }
    else Z = 0.0;*/
    Z = 1;

    C( i, 0 ) = c;
    R( i, 0 ) = r;

    /*M( i, 1 ) = Y;
    M( i, 2 ) = X;
    M( i, 3 ) = Y * X;
    M( i, 4 ) = Y * Y;
    M( i, 5 ) = X * X;
    M( i, 6 ) = -r * Y;
    M( i, 7 ) = -r * X;
    M( i, 8 ) = -r * Y * X;
    M( i, 9 ) = -r * Y * Y;
    M( i, 10 ) = -r * X * X;

    N( i, 1 ) = Y;
    N( i, 2 ) = X;
    N( i, 3 ) = Y * X;
    N( i, 4 ) = Y * Y;
    N( i, 5 ) = X * X;
    N( i, 6 ) = -c * Y;
    N( i, 7 ) = -c * X;
    N( i, 8 ) = -c * Y * X;
    N( i, 9 ) = -c * Y * Y;
    N( i, 10 ) = -c * X * X;*/

    /* for ( int mm = 11; mm < 39; mm++ )
    {
     M( i, mm ) = 1;
     N( i, mm ) = 1;
    }*/

    M( i, 1 ) = Z;
    M( i, 2 ) = Y;
    M( i, 3 ) = X;
    M( i, 4 ) = Z * Y;
    M( i, 5 ) = Z * X;
    M( i, 6 ) = Y * X;
    M( i, 7 ) = Z * Z;
    M( i, 8 ) = Y * Y;
    M( i, 9 ) = X * X;
    M( i, 10 ) = Z * Y * X;
    M( i, 11 ) = Z * Z * Y;
    M( i, 12 ) = Z * Z * X;
    M( i, 13 ) = Y * Y * Z;
    M( i, 14 ) = Y * Y * X;
    M( i, 15 ) = Z * X * X;
    M( i, 16 ) = Y * X * X;
    M( i, 17 ) = Z * Z * Z;
    M( i, 18 ) = Y * Y * Y;
    M( i, 19 ) = X * X * X;
    M( i, 20 ) = -r * Z;
    M( i, 21 ) = -r * Y;
    M( i, 22 ) = -r * X;
    M( i, 23 ) = -r * Z * Y;
    M( i, 24 ) = -r * Z * X;
    M( i, 25 ) = -r * Y * X;
    M( i, 26 ) = -r * Z * Z;
    M( i, 27 ) = -r * Y * Y;
    M( i, 28 ) = -r * X * X;
    M( i, 29 ) = -r * Z * Y * X;
    M( i, 30 ) = -r * Z * Z * Y;
    M( i, 31 ) = -r * Z * Z * X;
    M( i, 32 ) = -r * Y * Y * Z;
    M( i, 33 ) = -r * Y * Y * X;
    M( i, 34 ) = -r * Z * X * X;
    M( i, 35 ) = -r * Y * X * X;
    M( i, 36 ) = -r * Z * Z * Z;
    M( i, 37 ) = -r * Y * Y * Y;
    M( i, 38 ) = -r * X * X * X;

    N( i, 1 ) = Z;
    N( i, 2 ) = Y;
    N( i, 3 ) = X;
    N( i, 4 ) = Z * Y;
    N( i, 5 ) = Z * X;
    N( i, 6 ) = Y * X;
    N( i, 7 ) = Z * Z;
    N( i, 8 ) = Y * Y;
    N( i, 9 ) = X * X;
    N( i, 10 ) = Z * Y * X;
    N( i, 11 ) = Z * Z * Y;
    N( i, 12 ) = Z * Z * X;
    N( i, 13 ) = Y * Y * Z;
    N( i, 14 ) = Y * Y * X;
    N( i, 15 ) = Z * X * X;
    N( i, 16 ) = Y * X * X;
    N( i, 17 ) = Z * Z * Z;
    N( i, 18 ) = Y * Y * Y;
    N( i, 19 ) = X * X * X;
    N( i, 20 ) = -c * Z;
    N( i, 21 ) = -c * Y;
    N( i, 22 ) = -c * X;
    N( i, 23 ) = -c * Z * Y;
    N( i, 24 ) = -c * Z * X;
    N( i, 25 ) = -c * Y * X;
    N( i, 26 ) = -c * Z * Z;
    N( i, 27 ) = -c * Y * Y;
    N( i, 28 ) = -c * X * X;
    N( i, 29 ) = -c * Z * Y * X;
    N( i, 30 ) = -c * Z * Z * Y;
    N( i, 31 ) = -c * Z * Z * X;
    N( i, 32 ) = -c * Y * Y * Z;
    N( i, 33 ) = -c * Y * Y * X;
    N( i, 34 ) = -c * Z * X * X;
    N( i, 35 ) = -c * Y * X * X;
    N( i, 36 ) = -c * Z * Z * Z;
    N( i, 37 ) = -c * Y * Y * Y;
    N( i, 38 ) = -c * X * X * X;

  }

  QgsMatrix* result = NULL;
  QgsMatrix* pM = &M;
  QgsMatrix Mt( 39, n );
  result = QgsMatrix::transpose( &Mt, pM );
  QgsMatrix A1( 39, 39 );
  result = QgsMatrix::multiply( &A1, &Mt, pM );

  QgsMatrix* pR = &R;
  QgsMatrix B1( 39, 1 );
  result = QgsMatrix::multiply( &B1, &Mt, pR );

  QgsMatrix *pN = &N;
  QgsMatrix Nt( 39, n );
  result = QgsMatrix::transpose( &Nt, pN );
  QgsMatrix A2( 39, 39 );
  result =  QgsMatrix::multiply( &A2, &Nt, pN );

  QgsMatrix* pC = &C;
  QgsMatrix B2( 39, 1 );
  result = QgsMatrix::multiply( &B2, &Nt, pC );




  //set polynomial matrices J and K
  QgsMatrix Jm( 39, 1 );
  QgsMatrix Km( 39, 1 );
  QgsMatrix::solve( &Jm, &A1, &B1 );
  QgsMatrix::solve( &Km, &A2, &B2 );


  /*QgsMatrix* result = NULL;
  QgsMatrix* pM = &M;
  QgsMatrix Mt( 11, n );
  result = QgsMatrix::transpose( &Mt, pM );
  QgsMatrix A1( 11, 11 );
  result = QgsMatrix::multiply( &A1, &Mt, pM );

  QgsMatrix* pR = &R;
  QgsMatrix B1( 11, 1 );
  result = QgsMatrix::multiply( &B1, &Mt, pR );

  QgsMatrix *pN = &N;
  QgsMatrix Nt( 11, n );
  result = QgsMatrix::transpose( &Nt, pN );
  QgsMatrix A2( 11, 11 );
  result =  QgsMatrix::multiply( &A2, &Nt, pN );

  QgsMatrix* pC = &C;
  QgsMatrix B2( 11, 1 );
  result = QgsMatrix::multiply( &B2, &Nt, pC );

  //set polynomial matrices J and K
  QgsMatrix Jm( 11, 1 );
  QgsMatrix Km( 11, 1 );
  if ( NULL == QgsMatrix::solve( &Jm, &A1, &B1 ) ||
       NULL == QgsMatrix::solve( &Km, &A2, &B2 ) )
  {
    QgsLogger::debug( "System of equations are unsolvable" );
    return false;
  }*/


  /*QgsMatrix* result = NULL;
  QgsMatrix* pM = &M;
  QgsMatrix Mt( 11, n );
  result = QgsMatrix::transpose( &Mt, pM );
  QgsMatrix A1( 11, 11 );
  result = QgsMatrix::multiply( &A1, &Mt, pM );

  QgsMatrix* pR = &R;
  QgsMatrix B1( 11, 1 );
  result = QgsMatrix::multiply( &B1, &Mt, pR );

  QgsMatrix *pN = &N;
  QgsMatrix Nt( 11, n );
  result = QgsMatrix::transpose( &Nt, pN );
  QgsMatrix A2( 11, 11 );
  result =  QgsMatrix::multiply( &A2, &Nt, pN );

  QgsMatrix* pC = &C;
  QgsMatrix B2( 11, 1 );
  result = QgsMatrix::multiply( &B2, &Nt, pC );

  //set polynomial matrices J and K
  QgsMatrix Jm( 11, 1 );
  QgsMatrix Km( 11, 1 );

  QgsMatrix::solve( &Jm, &A1, &B1 );
  QgsMatrix::solve( &Km, &A2, &B2 );*/
  /*if ( NULL == QgsMatrix::solve( &Jm, &A1, &B1 ) ||
       NULL == QgsMatrix::solve( &Km, &A2, &B2 ) )
  {
    QgsLogger::debug( "System of equations are unsolvable" );
    return false;
  }*/

  for ( int i = 0; i < 39; i++ )
  {
    J[i] = Jm( i, 0 );
    K[i] = Km( i, 0 );
  }

  if ( !( J == NULL || K == NULL ) ) return true;
  else return false;
}

bool QgsRpcModel::initializeNoDem( )
{
  QList<QgsGcp*> list = mGcpSet->list();
  int n = list.size();

  //initialise matrices
  QgsMatrix R = QgsMatrix( n, 1 );
  QgsMatrix C = QgsMatrix( n, 1 );
  QgsMatrix M = QgsMatrix( n, 11 );
  QgsMatrix N = QgsMatrix( n, 11 );

  for ( int m = 0; m < n; m++ )
  {
    M( m, 0 ) = 1;
    N( m, 0 ) = 1;
  }

  for ( int i = 0; i < n; i++ )
  {
    QgsGcp* gcp = list.at( i );
    //get and transform r, c to pixel values relative to raw image
    double rGeo = gcp->rawY();
    double cGeo = gcp->rawX();
    int rPixel, cPixel;
    mImageData->transformCoordinate( cPixel, rPixel, cGeo, rGeo, QgsRasterDataset::TM_GEOPIXEL );
    //scale r, c (pixel) values to range [-1, 1]
    //printf("cPixel: %d, rPixel: %d\n",cPixel, rPixel);
    //printf("mImageYSize: %f, mImageXSize: %f\n",mImageYSize, mImageXSize);
    double r = ( rPixel / ( mImageYSize / 2.0 ) ) - 1;
    double c = ( cPixel / ( mImageXSize / 2.0 ) ) - 1;
    //printf("c: %f, r: %f\n",c, r);

    //get X,Y (geographical) values from GCP
    double X = gcp->refX();
    double Y = gcp->refY();
    //scale to range [-1,1] using original image dimensions
    X = (( X - mGeoTopLeftX ) / (( mGeoWidth ) / 2.0 ) ) - 1;
    Y = (( Y - mGeoTopLeftY ) / (( mGeoHeight ) / 2.0 ) ) - 1;

    C( i, 0 ) = c;
    R( i, 0 ) = r;

    M( i, 1 ) = Y;
    M( i, 2 ) = X;
    M( i, 3 ) = Y * X;
    M( i, 4 ) = Y * Y;
    M( i, 5 ) = X * X;
    M( i, 6 ) = -r * Y;
    M( i, 7 ) = -r * X;
    M( i, 8 ) = -r * Y * X;
    M( i, 9 ) = -r * Y * Y;
    M( i, 10 ) = -r * X * X;

    N( i, 1 ) = Y;
    N( i, 2 ) = X;
    N( i, 3 ) = Y * X;
    N( i, 4 ) = Y * Y;
    N( i, 5 ) = X * X;
    N( i, 6 ) = -c * Y;
    N( i, 7 ) = -c * X;
    N( i, 8 ) = -c * Y * X;
    N( i, 9 ) = -c * Y * Y;
    N( i, 10 ) = -c * X * X;
  }

  QgsMatrix* result = NULL;
  QgsMatrix* pM = &M;
  QgsMatrix Mt( 11, n );
  result = QgsMatrix::transpose( &Mt, pM );
  QgsMatrix A1( 11, 11 );
  result = QgsMatrix::multiply( &A1, &Mt, pM );

  QgsMatrix* pR = &R;
  QgsMatrix B1( 11, 1 );
  result = QgsMatrix::multiply( &B1, &Mt, pR );

  QgsMatrix *pN = &N;
  QgsMatrix Nt( 11, n );
  result = QgsMatrix::transpose( &Nt, pN );
  QgsMatrix A2( 11, 11 );
  result =  QgsMatrix::multiply( &A2, &Nt, pN );

  QgsMatrix* pC = &C;
  QgsMatrix B2( 11, 1 );
  result = QgsMatrix::multiply( &B2, &Nt, pC );

  //set polynomial matrices J and K
  QgsMatrix Jm( 11, 1 );
  QgsMatrix Km( 11, 1 );
  if ( NULL == QgsMatrix::solve( &Jm, &A1, &B1 ) ||
       NULL == QgsMatrix::solve( &Km, &A2, &B2 ) )
  {
    QgsLogger::debug( "System of equations are unsolvable" );
    return false;
  }

  for ( int i = 0; i < 11; i++ )
  {
    j[i] = Jm( i, 0 );
    k[i] = Km( i, 0 );
  }

  if ( !( j == NULL || k == NULL ) ) return true;
  else return false;
}
double QgsRpcModel::calculateRmsError( QgsGcp** pHighestDev, double* fHighestDev )
{
  if ( mInitialized && mGcpSet && mGcpSet->size() > 0 )
  {
    double fMaxDev = 0.0;
    QgsGcp* pMaxGcp = NULL;
    double fError;
    double fSquareError;
    double fSumOfErrors = 0.0;
    int pixelX, pixelY;
    double geoX, geoY;
    double C, R;
    typedef QList<QgsGcp*> GcpList;

    const GcpList& list = mGcpSet->constList();
    int nNumGcps = list.size();
    GcpList::const_iterator it = list.begin();
    for ( ; it != list.end(); ++it )
    {
      QgsGcp* pGcp = ( *it );

      geoX = pGcp->rawX();
      geoY = pGcp->rawY();
      mImageData->transformCoordinate( pixelX, pixelY, geoX, geoY, QgsRasterDataset::TM_GEOPIXEL );
      C = this->c( pGcp->refX(), pGcp->refY(), pGcp->refZ() );
      R = this->r( pGcp->refX(), pGcp->refY(), pGcp->refZ() );
      fError = sqrt( SQUARE( C - pixelX ) + SQUARE( R - pixelY ) ); //Calculate distance between two points
      fSquareError = SQUARE( fError );
      fSumOfErrors += fSquareError;
      if ( fSquareError > fMaxDev )
      {
        fMaxDev = fSquareError;
        pMaxGcp = pGcp;
      }
    }
    mRMSE = sqrt( fSumOfErrors / nNumGcps );
    if ( NULL != pHighestDev )
    {
      *pHighestDev = pMaxGcp;
      *fHighestDev = fMaxDev;
    }
    return mRMSE;
  }
  else
  {
    QgsLogger::debug( "RPC Model has not been initialized. Cannot calculate RMSE" );
    return 0.0;
  }
}

//double QgsRpcModel::refine( )
//{
//  if (mInitialized && (mGcpSet->size() > 0))
//  {
//    typedef QList<QgsGcp*> GcpList;
//    GcpList& list = mGcpSet->list();
//    GcpList::iterator it = list.begin();
//
//    double r_sq_sum;
//    double highest_err;
//    int badGcp = 0;
//
//    //QgsGcp* gcp = (*it);
//    if (mDem)
//    {
//      for (int i =0; i < list.size(); i++)
//      {
//        QgsGcp* gcp = list.at(i);
//        int rawX, rawY;
//        double geoX = gcp->rawX();
//        double geoY = gcp->rawY();
//        mImageData->transformCoordinate(rawX, rawY, geoX, geoY, QgsRasterDataset::TM_GEOPIXEL);
//        double err = (pow((rawX-c(gcp->refX(),gcp->refY(),gcp->refZ())),2) + pow((rawY-r(gcp->refX(),gcp->refY(),gcp->refZ())),2));
//        //printf("Error: %f\n", err);
//        r_sq_sum += err;
//        if (err > highest_err) { badGcp = i; highest_err = err; }
//      }
//    }
//    else
//    {
//      for (int i =0; i < list.size(); i++)
//      {
//        QgsGcp* gcp = list.at(i);
//        int rawX, rawY;
//        double geoX = gcp->rawX();
//        double geoY = gcp->rawY();
//        mImageData->transformCoordinate(rawX, rawY, geoX, geoY, QgsRasterDataset::TM_GEOPIXEL);
//        double err = (pow((rawX-c(gcp->refX(),gcp->refY(),0)),2) + pow((rawY-r(gcp->refX(),gcp->refY(),0)),2));
//        //printf("Error: %f\n", err);
//        r_sq_sum += err;
//        if (err > highest_err) { badGcp = i; highest_err = err; }
//      }
//    }
//
//    double rmse = sqrt(r_sq_sum/list.size());
//    printf("RMSE: %f\n", rmse);
//
//    /*if (rmse > RMSE_THRESHOLD) //TODO: decide how to decide on threshold value
//    {
//      list.removeAt(badGcp);
//      printf("Erased a bad gcp: %f\n", highest_err);
//      mRMSE = rmse;
//      if ( mDem ) mInitialized = initialize( gcpSet );
//      else mInitialized = initializeNoDem( gcpSet );
//    }*/
//
//    return rmse;
//  }
//  else return -1; //not initialized: some QgsDebugger message
//}

/*void QgsRpcModel::printMatrix( QgsMatrix M )
{
  for ( int i = 0; i < M.rows(); i++ )
  {
    printf( "| " );
    for ( int j = 0; j < M.columns(); j++ )
    {
      printf( " %f ", M( i, j ) );
    }
    printf( " |\n" );
  }
  printf( " \n" );
}

void QgsRpcModel::printMatrixJ()
{
  printf("{ ");
  for (int i=0; i < 11; i++)
  {
    printf("%f ",j[i]);
  }
  printf("}\n");
}

void QgsRpcModel::printMatrixK()
{
  printf("{ ");
  for (int i=0; i < 11; i++)
  {
    printf("%f ",k[i]);
  }
  printf("}\n");
}*/
