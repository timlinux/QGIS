/***************************************************************************
qgscrosscorrelator.cpp - This class handles the cross correlation of two
subimages.
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
/* $Id: qgscrosscorrelator.cpp 606 2010-10-29 09:13:39Z jamesm $ */

#include "qgscrosscorrelator.h"
#include "qgsrasterdataset.h"
#include "qgslogger.h"
#include <math.h>
#include <iostream>
#include <ios>

QgsCrossCorrelator::QgsCrossCorrelator( QgsRasterDataset* ref,  QgsRasterDataset* raw )
{
  refChip = ref;
  rawChip = raw;
}

QgsCrossCorrelator::~QgsCrossCorrelator()
{
}

double QgsCrossCorrelator::correlationValue()
{
  //find the band(s) with which to calculate each distinct image's grey level:
  int refbands = refChip->rasterBands();
  for ( int i = 0; i < refbands; i++ )
  {

    GDALRasterBand* band = refChip->gdalDataset()->GetRasterBand( i + 1 );
    GDALColorInterp color = band->GetColorInterpretation();

    switch ( color )
    {
      case GCI_GrayIndex:
      { greyRefBand = i + 1; refConvert = GREY; }
      break;
      case GCI_RedBand:
      { redRefBand = i + 1; refConvert = RGB; }
      break;
      case GCI_GreenBand:
      { greenRefBand = i + 1; refConvert = RGB; }
      break;
      case GCI_BlueBand:
      { blueRefBand = i + 1; refConvert = RGB; }
      break;
      case GCI_CyanBand:
      { cyanRefBand = i + 1; refConvert = CMYK; }
      break;
      case GCI_MagentaBand:
      { magentaRefBand = i + 1; refConvert = CMYK; }
      break;
      case GCI_YellowBand:
      { yellowRefBand = i + 1;  refConvert = CMYK; }
      break;
      case GCI_BlackBand:
      { blackRefBand = i + 1;  refConvert = CMYK; }
      break;
      default:
      {
        if ( refbands < 3 )
        {
          greyRefBand = 1; refConvert = GREY;
        }
        else
        {
          redRefBand = 1; greenRefBand =  2; blueRefBand = 3; refConvert = RGB;
        }
      }
      break; //ASSUME RGB
    }
    if ( color == GCI_GrayIndex ) break;
  }

  int rawbands = rawChip->rasterBands();
  for ( int j = 0; j < rawbands; j++ )
  {
    GDALRasterBand* band = rawChip->gdalDataset()->GetRasterBand( j + 1 );
    GDALColorInterp color = band->GetColorInterpretation();

    switch ( color )
    {
      case GCI_GrayIndex:
      { greyRawBand = j + 1; rawConvert = GREY; }
      break;
      case GCI_RedBand:
      { redRawBand = j + 1; rawConvert = RGB; }
      break;
      case GCI_GreenBand:
      { greenRawBand = j + 1; rawConvert = RGB; }
      break;
      case GCI_BlueBand:
      { blueRawBand = j + 1; rawConvert = RGB; }
      break;
      case GCI_CyanBand:
      { cyanRawBand = j + 1; rawConvert = CMYK; }
      break;
      case GCI_MagentaBand:
      { magentaRawBand = j + 1; rawConvert = CMYK; }
      break;
      case GCI_YellowBand:
      { yellowRawBand = j + 1; rawConvert = CMYK; }
      break;
      case GCI_BlackBand:
      { blackRawBand = j + 1; rawConvert = CMYK; }
      break;
      default:
      {
        if ( rawbands < 3 )
        {
          greyRawBand = 1; rawConvert = GREY;
        }
        else
        {
          redRawBand = 1; greenRawBand =  2; blueRawBand = 3; rawConvert = RGB;
        }
      }
      break; //ASSUME RGB
    }
    if ( color == GCI_GrayIndex ) break;
  }
  calculateAverages();
  return calculateCC();
}

//assumes that bands and Schemes have been set
void QgsCrossCorrelator::calculateAverages()
{
  refAvg = 0;
  rawAvg = 0;
  for ( int i = 0; i < refChip->imageXSize(); i++ )
  {

    for ( int j = 0; j < refChip->imageYSize(); j++ )
    {
      refAvg += greyLevel( true, i, j );
    }
  }
  refAvg = refAvg / ( refChip->imageXSize() * refChip->imageYSize() );

  for ( int i = 0; i < rawChip->imageXSize(); i++ )
  {

    for ( int j = 0; j < rawChip->imageYSize(); j++ )
    {
      rawAvg += greyLevel( false, i, j );
    }
  }
  rawAvg = rawAvg / ( rawChip->imageXSize() * rawChip->imageYSize() );
}
double QgsCrossCorrelator::correlationCoefficent( double* srcData, double* destData, int xSize, int ySize )
{
  double srcAvg = 0.0,
                  destAvg = 0.0;
  //Calculate averages
  int fullSize = xSize * ySize;
  for ( int i = 0; i < fullSize; ++i )
  {
    srcAvg += srcData[i];
    destAvg += destData[i];
  }

  srcAvg /= fullSize;
  destAvg /= fullSize;
  //Calculate correlation value;
  double srcDev;
  double destDev;
  double numerator = 0.0;

  double srcDenom = 0.0,
                    destDenom = 0.0;
  for ( int i = 0; i < fullSize; ++i )
  {
    srcDev = srcData[i] - srcAvg;
    destDev = destData[i] - destAvg;
    numerator += srcDev * destDev;

    srcDenom += srcDev * srcDev;
    destDenom += destDev * destDev;
  }
  double denom = sqrt( srcDenom * destDenom );

  return numerator / denom;
}

double QgsCrossCorrelator::calculateCC()
{
  double num = 0.0;
  double a = 0.0;
  double b = 0.0;
  for ( int i = 0; i < refChip->imageXSize(); i++ )
  {
    for ( int j = 0; j < refChip->imageYSize(); j++ )
    {
      //(ref's gray level - ref's avg gray level)*(raw's gray level - raw's avg gray level)
      num += ( greyLevel( true, i, j ) - refAvg ) * ( greyLevel( false, i, j ) - rawAvg );
      //(ref's gray level - ref's avg gray level)
      a += pow(( greyLevel( true, i, j ) - refAvg ), 2 );
      //(raw's gray level - raw's avg gray level)
      b += pow(( greyLevel( false, i, j ) - rawAvg ), 2 );
    }
  }

  double denom = pow(( a * b ), 0.5 );
  return num / denom;
}

double QgsCrossCorrelator::greyLevel( bool isRef, int x, int y )
{
  if ( isRef )
  {


    if ( refConvert == GREY )
    {

      return refChip->readValue( greyRefBand, x, y );
    }

    if ( refConvert == RGB )
    {
      return ( refChip->readValue( redRefBand, x, y )*0.299 + refChip->readValue( greenRefBand, x, y )*0.587 + refChip->readValue( blueRefBand, x, y )*0.114 );
    }
    else if ( refConvert == CMYK )
    {
      return ( refChip->readValue( cyanRefBand, x, y )*0.299 + refChip->readValue( magentaRefBand, x, y )*0.587 + refChip->readValue( yellowRefBand, x, y )*0.114 + refChip->readValue( blackRefBand, x, y ) );
    }
  }

  else
  {

    if ( rawConvert == GREY ) return rawChip->readValue( greyRawBand, x, y );

    if ( rawConvert == RGB )
    {
      return ( rawChip->readValue( redRawBand, x, y )*0.299 + rawChip->readValue( greenRawBand, x, y )*0.587 + rawChip->readValue( blueRawBand, x, y )*0.114 );
    }
    else if ( rawConvert == CMYK )
    {
      return ( rawChip->readValue( cyanRawBand, x, y )*0.299 + rawChip->readValue( magentaRawBand, x, y )*0.587 + rawChip->readValue( yellowRawBand, x, y )*0.114 + rawChip->readValue( blackRawBand, x, y ) );
    }
    else return 0.0;
  }
}


