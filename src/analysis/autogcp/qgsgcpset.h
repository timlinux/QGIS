/***************************************************************************
qgsgcpset.cpp - This is a container class that represents an entire set of
 GCPs associated with a specific reference model or a specific extraction
 and cross-referencing operation.
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
/* $Id: qgsgcpset.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSGCPSET_H
#define QGSGCPSET_H


//****************Includes
#include <QList> //Internal container
#include "qgsgcp.h"
class QgsGcp;
/*!\ingroup analysis
 *
 * This is a container class that represents an entire set of
 * GCPs associated with a specific reference model or a specific extraction
 * and cross-referencing operation.
 */
class ANALYSIS_EXPORT QgsGcpSet
{
  public:
    /*! \brief QgsGcpSet constructor
    Creates an empty GCP set.
    */
    QgsGcpSet();
    /*! \brief QgsGcpSet copy constructor
     *
     *  Copies all GCPs to this set
     */
    QgsGcpSet( const QgsGcpSet& set );
    /*! \brief QgsGcpSet destructor
     *
     *  Destroys all contained QgsGcp objects
     */
    virtual ~QgsGcpSet();
    /*! \brief Gets an ID associated with this ref GCP dataset
       * Usually this is for database purposes, but can be used for other forms of unique identification.
       * \return Returns the ref ID of this GCP dataset or -1 if no ID has been set.
       */
    int refId() const { return mRefId; }
    /*! \brief Sets the ref ID of this GCP dataset
     */
    void setRefId( int id ) { mRefId = id; }
    /*! \brief Gets an ID associated with this raw GCP dataset
       * Usually this is for database purposes, but can be used for other forms of unique identification.
       * \return Returns the raw ID of this GCP dataset or -1 if no ID has been set.
       */
    int rawId() const { return mRawId; }
    /*! \brief Sets the raw ID of this GCP dataset
     */
    void setRawId( int id ) { mRawId = id; }
    /*! \brief Adds a GCP to this GCP set.
    May add duplicate entries.
    */
    void addGcp( QgsGcp* point );
    /*! \brief Updates a GCP in this GCP set.
    */
    void updateGcp( QgsGcp* oldPoint, QgsGcp* newPoint );
    /*! \brief  Removes a GCP from the set.
    This removes the first GCP in the list with match reference x and y coordinates.
    */
    void removeGcp( QgsGcp* point );
    /*! \brief Gets the amount of GCP's in the list
    */
    int size() const;
    /*! \brief Removes all GCPs from this GCP set
    */
    void clear();
    /*! \brief  Returns the underlying QList
    */
    const QList<QgsGcp*>& constList() const;
    /*! \brief Returns a reference to the underlying QList
    */
    QList<QgsGcp*>& list();
  private:
    typedef QList<QgsGcp*> GcpList;
    GcpList mList;
    int mRawId;
    int mRefId;

    //Functor used to order the list in descending order
    //edit: Not used at the moment.

};

#endif // QGSGCPSET_H
