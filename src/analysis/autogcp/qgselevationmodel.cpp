/***************************************************************************
qgselevationmodel.cpp - The ElevationModel contains a Digital Elevation
 Model of a specific reference image and allows sampling of the ground
 elevation at different coordinates. This DEM is used in the orthorectification
 process as well as chip projection.
--------------------------------------
Date : 07-May-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgselevationmodel.cpp 506 2010-10-16 14:02:25Z jamesm $ */
#include "qgselevationmodel.h"

QgsElevationModel::QgsElevationModel( QString filename )
    : QgsRasterDataset( filename, QgsRasterDataset::Update ), calculated( false ), highest( 0.0 ), lowest( 0.0 )
{
  mDataset = QgsRasterDataset::gdalDataset();
}

bool QgsElevationModel::elevateGeographicPoint( const double &geoX, const double &geoY, double &geoZ )
{
  if ( !mDataset )
  {
    QgsLogger::debug( "QgsElevationModel: Invalid DEM dataset pointer." );
    return false;
  }

  if (( geoX > 0 ) && ( geoY > 0 ) )
  {
    int pixelX;
    int pixelY;
    double gX = geoX;
    double gY = geoY;
    transformCoordinate( pixelX, pixelY, gX, gY, QgsRasterDataset::TM_GEOPIXEL );
    geoZ = readValue( EM_BANDNO, pixelX, pixelY );
    return true;
  }
  else
  {
    return false;
  }
}

bool QgsElevationModel::elevatePixelPoint( const int pixelX, const int pixelY, double &geoZ )
{
  if ( !mDataset )
  {
    QgsLogger::debug( "QgsElevationModel: Invalid DEM dataset pointer." );
    return false;
  }

  if (( pixelX > 0 ) && ( pixelY > 0 ) )
  {
    //transformCoordinate(pixelX, pixelY, geoX, geoY, QgsRasterDataset::TM_PIXELGEO);
    geoZ = readValue( EM_BANDNO, pixelX, pixelY );
    return true;
  }
  else
  {
    QgsLogger::debug( "QgsElevationModel: Row and Column values." );
    return false;
  }

}

double QgsElevationModel::highestElevationValue()
{
  if ( !mDataset )
  {
    QgsLogger::debug( "QgsElevationModel: Invalid DEM dataset pointer." );
    return -1.0;
  }

  if ( !calculated )
  {
    calculated = calculateExtremes();
    return highest;
  }
  return highest;
}

double QgsElevationModel::lowestElevationValue()
{
  if ( !mDataset )
  {
    QgsLogger::debug( "QgsElevationModel: Invalid DEM dataset pointer." );
    return 0.0;
  }
  if ( !calculated )
  {
    calculated = calculateExtremes();
    return lowest;
  }
  return lowest;

}


bool QgsElevationModel::calculateExtremes()
{
  highest = readValue( EM_BANDNO, 1, 1 );
  lowest = readValue( EM_BANDNO, 1, 1 );
  if ( !mDataset )
  {
    QgsLogger::debug( "QgsElevationModel: Invalid DEM dataset pointer." );
    return false;
  }
  int nPixels = mDataset->GetRasterXSize();
  int nLines = mDataset->GetRasterYSize();

  for ( int x = 1; x < nPixels; x++ )
  {
    for ( int y = 1; y < nLines; y++ )
    {
      double val = readValue( EM_BANDNO, x, y );
      if ( val > highest )
      {
        highest = val;
      }
      if ( val < lowest )
      {
        lowest = val;
      }//ifelse

    }//for2
  }//for1
  return true;
}
