/***************************************************************************
 *   Copyright (C) 2005 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef QGSPROJECTIONSELECTOR_H
#define QGSPROJECTIONSELECTOR_H

#ifdef WIN32
#include "qgsprojectionselectorbase.uic.h"
#else
#include "qgsprojectionselectorbase.h"
#endif

#include <qgis.h>
#include <qstring.h>
#include <qlistview.h>


/**
@author Tim Sutton
*/
class QgsProjectionSelector: public QgsProjectionSelectorBase
{
Q_OBJECT
public:
    QgsProjectionSelector( QWidget* parent , const char* name ,WFlags fl =0  );
    ~QgsProjectionSelector();
  //! Populate the wkts map with projection names...
  void getProjList();
  
    
public slots:
    void setSelectedWKT(QString theWKT);
    QString getSelectedWKT();
    void setSelectedSRID(QString theSRID);
    QString getCurrentWKT();
    QString getCurrentSRID();
    
private:
  //! map containing SPATIAL_REF_SYS items keyed by the WKT name
  // XXX why are we using QMap instead of std::map ?
  //XXX List view items for the tree view of projections
  //! GEOGCS node
  QListViewItem *geoList;
  //! PROJCS node
  QListViewItem *projList;
  //! Users custom coordinate system file
  QString customCsFile;

  //private handler for when user selects a cs
  //it will cause wktSelected and sridSelected events to be spawned
  void coordinateSystemSelected(QListViewItem*);
  
signals:
    void wktSelected(QString theWKT);
    void sridSelected(QString theSRID);
};

#endif
