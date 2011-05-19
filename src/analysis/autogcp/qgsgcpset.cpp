/***************************************************************************
qgsgcpset.cpp - This is a container class that represents an entire set of
 GCP�s associated with a specific reference model or a specific extraction
 and cross-referencing operation.
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
/* $Id: qgsgcpset.cpp 606 2010-10-29 09:13:39Z jamesm $ */
#include "qgsgcpset.h"

QgsGcpSet::QgsGcpSet()
{
  mRefId = 0;
  mRawId = 0;
}

QgsGcpSet::~QgsGcpSet()
{
  GcpList::iterator it = mList.begin();
  for ( ; it != mList.end(); ++it )
  {
    delete( *it );
  }
}

QgsGcpSet::QgsGcpSet( const QgsGcpSet& set )
{
  mRefId = set.refId();
  mRawId = set.rawId();
  const QList<QgsGcp*>& list = set.constList();
  for ( int i = 0; i < list.size(); i++ )
  {
    addGcp( new QgsGcp( *list.at( i ) ) );
  }
}

//Adds the GCP's in descending order of error value
void QgsGcpSet::addGcp( QgsGcp* point )
{
  mList.append( point );
}
void QgsGcpSet::removeGcp( QgsGcp* point )
{
  GcpList::iterator it = mList.begin();
  for ( ; it != mList.end(); it++ )
  {
    if (( *it ) == point )
    {
      mList.erase( it );
      break;
    }
  }
}
//Sorts all GCP's in descending order of error values
/*void QgsGcpSet::sort()
{
  DescErrorCompare comp;
  mList.sort(comp);
}*/

void QgsGcpSet::updateGcp( QgsGcp* oldPoint, QgsGcp* newPoint )
{
  GcpList::iterator it = mList.begin();
  for ( ; it != mList.end(); it++ )
  {
    if (( *it ) == oldPoint )
    {
      ( *it ) = newPoint;
      break;
    }
  }
}

void QgsGcpSet::clear()
{
  mList.clear();
}


int QgsGcpSet::size() const
{
  return mList.size();
}

const QList<QgsGcp*>& QgsGcpSet::constList() const
{
  return mList;
}

QList<QgsGcp*>& QgsGcpSet::list()
{
  return mList;
}
