/***************************************************************************
qgsgcp.h - A Ground Control Point (GCP) abstraction contains information about
 a specific GCP such as the ground and associated image coordinates of this
 GCP, as well as additional elevation and other information. This object is
 also the container for the GCP chip data.
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
/* $Id: qgsgcp.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSGCP_H
#define QGSGCP_H
#include "qgsrasterdataset.h"
/*! \ingroup analysis
 *
 * A Ground Control Point (GCP) abstraction contains information about
 * a specific GCP such as the ground and associated image coordinates of this
 * GCP, as well as additional elevation and other information. This object is
 * also the container for the GCP chip data.
 */
class ANALYSIS_EXPORT QgsGcp
{
  public:
    /*! \brief Contructs an empty QgsGcp.
     *
     * The GCP is initialized with all zero values
     */
    QgsGcp();
    /*! \brief Copy constructor
     *
     * @note The copy constructor does not perform a deep copy by cloning the image chip.
     * It merely sets the new GCP's chip to NULL
     */
    QgsGcp( const QgsGcp& other );
    /*! \brief QgsGcp destructor
     *
     * Releases the memory allocated for the GCP chip if it exists
     */
    ~QgsGcp();

    bool operator==( const QgsGcp& other ) const;
    /*! \brief Assignment Operator
    *
    * @note This does not perform a deep copy by cloning the image chip.
    * It merely sets the new GCP's chip to NULL
    *
    * \sa QgsGcp(const QgsGcp&)
    */
    const QgsGcp& operator=( const QgsGcp& other );
    /*! \brief Gets an ID associated with this GCP
     * Usually this is for database purposes, but can be used for other forms of unique identification.
     * \return Returns the ID of this GCP or -1 if no ID has been set.
     */
    int id() const { return mId; }
    /*! \brief Sets the ID of this GCP
     */
    void setId( int id ) { mId = id; }
    /*! \brief Returns the X-component of this GCP's geographic coordinates.
     */
    double refX() const;
    /*! \brief Sets the X-component of this GCP's geographic coordinates.
     */
    void setRefX( double value );
    /*! \brief Returns the Y-component of this GCP's geographic coordinates.
     */
    double refY() const;
    /*! \brief Sets the Y-component of this GCP's geographic coordinates.
     */
    void setRefY( double value );
    /*! \brief Returns the Z-component of this GCP's geographic coordinates.
     * This is usually either 0 or the height as loaded from a Digital Elevation Model
     */
    double refZ() const;
    /*! \brief Sets the Z-component of this GCP's geographic coordinates.
     */
    void setRefZ( double value );
    /*! \brief Gets the pixel column of this point on the associated image
     */
    double rawX() const;
    /*! \brief Sets the pixel column of this point on the associated image
     */
    void setRawX( double value );
    /*! \brief Gets the pixel line of this point on the associated image
     */
    double rawY() const;
    /*! \brief Sets the pixel line of this point on the associated image
     */
    void setRawY( double value );
    double correlationCoefficient() const;
    void setCorrelationCoefficient( double value );
    /*!\brief Gets the GCP chip
     * The returned pointer must not be deleted while the GCP is still being used.
     * \return Returns a pointer to the chip as a QgsImagedataset, or NULL if no chip has been extracted.
     */
    QgsRasterDataset* gcpChip() const;
    /*!\brief Sets the GCP chip
     */
    void setGcpChip( QgsRasterDataset* value );
  protected:

  private:
    double mXRaw, mYRaw, mXRef, mYRef, mZRef;
    int mId;
    double mCorrelationCoefficient;
    QgsRasterDataset* mChip;

};

#endif // QGSGCP_H
