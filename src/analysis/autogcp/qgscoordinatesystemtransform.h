/***************************************************************************
qgscoordinatesystemtransform.h - Converts coordinates between a source and
destination coordinate system
--------------------------------------
Date : 24-October-2010
Author : James Meyer
Copyright : (C) 2010 by C.J. Meyer
Email : jamesmeyerx@gmail.com

/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgscoordinatesystemtransform.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSCOORDINATESYSTEMTRANSFORM_H
#define QGSCOORDINATESYSTEMTRANSFORM_H

#include <ogr_spatialref.h>
#include <QString>
/*! \brief Class for converting ground coordinates between two coordinate reference systems
 *
 * This class is a simplified standalone version of the QgsCoordinateTransform
 * that works directly with the OGR projection classes.
 */
class ANALYSIS_EXPORT QgsCoordinateSystemTransform
{
  public:
    /*! \brief Defaults Constructor
     *
     * Construct a the transform with no initial source or destination CRS
     */
    QgsCoordinateSystemTransform();
    /*! \brief Constructs the transform with the given source and destination CRS*/
    QgsCoordinateSystemTransform( const QString& srcWkt, const QString& dstWkt );
    /*! \brief Destructor
     *
     * Releases the internal OGR handles created for transformations.
     */
    ~QgsCoordinateSystemTransform();
    /*! \brief In-place transformation of a set of coordinates
     *
     * \param count The number of coordinates to transform
     * \param xCoords A pointer to the first X-Coordinate to transform
     * \param yCoords A pointer to the first Y-Coordinate to transform
     */
    bool transform( int count, double* xCoords, double* yCoords );
    /*! \brief Converts a single pair of coordinates
     *
     * \param srcX The source x-coordinate
     * \param srcY The source y-coordinate
     * \param dstX The address where the transformed X-coordinate will be stored
     * \param dstY The address where the transformed Y-coordinate will be stored
     * \return True if the transformation succeeded, false otherwise
     */
    bool transform( double srcX, double srcY, double* dstX, double* dstY );
    /*! \brief Sets the source Wkt coordinate reference string*/
    void setSourceCrs( const QString& srcWkt );
    /*! \brief Sets the destination Wkt coordinate reference string*/
    void setDestinationCrs( const QString& dstWkt );
    /*!\brief Gets the source Wkt coordinate reference string*/
    const QString& sourceCrs() const;
    /*!\brief Gets the destination Wkt coordinate reference string*/
    const QString& destinationCrs() const;
  private:
    bool createTransformer();
  private:
    OGRSpatialReference mSrcCrs;
    OGRSpatialReference mDstCrs;
    QString mSrcString;
    QString mDstString;
    OGRCoordinateTransformation *mTransformer;
    bool mInitialized;
    bool mSrcSet, mDstSet;
};


#endif  /* QGSCOORDINATESYSTEMTRANSFORM_H */

