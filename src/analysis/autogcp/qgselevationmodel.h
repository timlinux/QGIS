/***************************************************************************
qgselevationmodel.cpp - The ElevationModel contains a Digital Elevation
 Model of a specific reference image and allows sampling of the ground
 elevation at different coordinates. This DEM is used in the orthorectification
 process as well as chip projection.
--------------------------------------
Date : 15-September-2010
Author: Francois Maass
Copyright : (C) 2010 by Foxhat Solutions
Email : foxhat.solutions@gmail.com
***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgselevationmodel.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSELEVATIONMODEL_H
#define QGSELEVATIONMODEL_H
#include "qgsrasterdataset.h"
#include <qgslogger.h>
#include <QString>

/*! \ingroup analysis*/

/*! \brief An elevation model.
 * The elevation model is used to retrieved height/ elevation values for specified points from a Digital Elevation Model.
 */
class ANALYSIS_EXPORT QgsElevationModel : public QgsRasterDataset
{
  public:
    /*! \brief Creates a Elevation Model Dataset
    * \param filename The path to the DEM file.
    */
    QgsElevationModel( QString filename );
    /*! \brief Determine the elevation of the specified geographic point
    * \param geoX The geographic X coordinate
    * \param geoY The geographic Y coordinate
    * \param geoZ The geographic Z coordinate that will be determined
    */
    bool elevateGeographicPoint( const double &geoX, const double &geoY, double &geoZ );
    /*! \brief Determine the elevation of the specified pixel point
    * \param pixelX The pixel X coordinate
    * \param pixelY The pixel Y coordinate
    * \param geoZ The geographic Z coordinate that will be determined
    */
    bool elevatePixelPoint( const int pixelX, const int pixelY, double &geoZ );
    /*! \brief Determines the highest elevation value in the DEM
    */
    double highestElevationValue();
    /*! \brief Determines the lowest elevation value in the DEM
    */
    double lowestElevationValue();
    /*! \brief Constant value for DEM band number
    */
    static const int EM_BANDNO = 1;
  private:
    bool calculateExtremes();
    bool calculated;
    double highest;
    double lowest;
};

#endif //QGSELEVATIONMODEL
