/***************************************************************************
 *   Copyright (C) 2005 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "qgsprojectionselector.h"

//standard includes
#include <iostream>
#include <cassert>

//qgis includes
#include "qgsspatialreferences.h"
#include "qgscsexception.h"
#include "qgsconfig.h"

//qt includes
#include <qapplication.h>
#include <qfile.h>
#include <qtextedit.h>
#include <qbuttongroup.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qregexp.h>
#include <qprogressdialog.h> 
#include <qapplication.h>

//stdc++ includes
#include <iostream>
#include <cstdlib>

//gdal and ogr includes
#include <ogr_api.h>
#include <ogr_spatialref.h>
#include <cpl_error.h>

// set the default coordinate system
static const char* defaultWktKey = "4326";

QgsProjectionSelector::QgsProjectionSelector( QWidget* parent , const char* name , WFlags fl  )
    : QgsProjectionSelectorBase( parent, "Projection Selector", fl )
{

  // Populate the projection list view
  getProjList();
}

QgsProjectionSelector::~QgsProjectionSelector()
{
}
void QgsProjectionSelector::setSelectedWKT(QString theWKT)
{
  //get the srid given the wkt so we can pick the correct list item
#ifdef QGISDEBUG
  std::cout << "QgsProjectionSelector::setSelectedWKT called with \n" << theWKT << std::endl;
#endif
  //now delegate off to the rest of the work
  QListViewItemIterator myIterator (lstCoordinateSystems);
  while (myIterator.current()) 
  {
    if (myIterator.current()->text(0)==theWKT)
    {
      lstCoordinateSystems->setCurrentItem(myIterator.current());
      lstCoordinateSystems->ensureItemVisible(myIterator.current());
      return;
    }
    ++myIterator;
  }
}

void QgsProjectionSelector::setSelectedSRID(QString theSRID)
{
  QListViewItemIterator myIterator (lstCoordinateSystems);
  while (myIterator.current()) 
  {
    if (myIterator.current()->text(1)==theSRID)
    {
      lstCoordinateSystems->setCurrentItem(myIterator.current());
      lstCoordinateSystems->ensureItemVisible(myIterator.current());
      return;
    }
    ++myIterator;
  }

}

//note this ine just returns the WKT NAME!
QString QgsProjectionSelector::getSelectedWKT()
{
  // return the selected wkt name from the list view
  QListViewItem *lvi = lstCoordinateSystems->currentItem();
  if(lvi)
  {
    return lvi->text(0);
  }
  else
  {
    return QString::null;
  }
}
//this one retunr the whole wkt
QString QgsProjectionSelector::getCurrentWKT()
{
  // Only return the projection if there is a node in the tree
  // selected that has an srid. This prevents error if the user
  // selects a top-level node rather than an actual coordinate
  // system
  QListViewItem *lvi = lstCoordinateSystems->currentItem();
  QgsSpatialRefSys *srs = QgsSpatialReferences::instance()->getSrsBySrid(lvi->text(1));
  if(srs)
  {
    //set the text box to show the full proection spec
#ifdef QGISDEBUG
    std::cout << "Current selected : " << lvi->text(0) << std::endl;
    std::cout << "Current full wkt : " << srs->srText() << std::endl;
#endif
    return srs->srText();
  }
  else
  {
    return NULL;
  }

}

QString QgsProjectionSelector::getCurrentSRID()
{
  if(lstCoordinateSystems->currentItem()->text(1).length() > 0)
  {
    return lstCoordinateSystems->currentItem()->text(1);
  }
  else
  {
    return NULL;
  }
}

void QgsProjectionSelector::getProjList()
{
  // setup the nodes for the list view
  //
  // Geographic coordinate system node
  geoList = new QListViewItem(lstCoordinateSystems,"Geographic Coordinate System");
  // Projected coordinate system node
  projList = new QListViewItem(lstCoordinateSystems,"Projected Coordinate System");

  // Get the instance of the spaital references object that
  // contains a collection (map) of all QgsSpatialRefSys objects
  // for every coordinate system in the resources/spatial_ref_sys.txt
  // file
  QgsSpatialReferences *srs = QgsSpatialReferences::instance();

  // Determine the current project projection so we can select the 
  // correct entry in the combo

  //
  // TODO ! Implemente this properly!!!! ts
  //
  QString currentSrid = ""; 
  // get the reference to the map
  projectionWKTMap_t mProjectionsMap = srs->getMap();
  // get an iterator for the map
  projectionWKTMap_t::iterator myIterator;
  //find out how many entries in the map
  int myEntriesCount = mProjectionsMap.size();
#ifdef QGISDEBUG
  std::cout << "Srs map has " << myEntriesCount << " entries " << std::endl;
#endif
  int myProgress = 1;
  QProgressDialog myProgressBar( "Building Projections List...", "Cancel", myEntriesCount,
          this, "progress", TRUE );
  myProgressBar.setProgress(myProgress);
  // now add each key to our list view
  QListViewItem *newItem;
  for ( myIterator = mProjectionsMap.begin(); myIterator != mProjectionsMap.end(); ++myIterator ) 
  {
    myProgressBar.setProgress(myProgress++);
    qApp->processEvents();
    //std::cout << "Widget map has: " <<myIterator.key().ascii() << std::endl;
    //cboProjection->insertItem(myIterator.key());

    QgsSpatialRefSys *srs = *myIterator;
    assert(srs != 0);
    //XXX Add to the tree view
    if(srs->isGeographic())
    {
      // this is a geographic coordinate system
      // Add it to the tree
      newItem = new QListViewItem(geoList, srs->name());
      //    std::cout << "Added " << getWKTShortName(srs->srText()) << std::endl; 
      // display the spatial reference id in the second column of the list view
      newItem->setText(1,srs->srid());
    }
    else
    {
      // coordinate system is projected...
      QListViewItem *node; // node that we will add this cs to...

      // projected coordinate systems are stored by projection type
      QStringList projectionInfo = QStringList::split(" - ", srs->name());
      if(projectionInfo.size() == 2)
      {
        // Get the projection name and strip white space from it so we
        // don't add empty nodes to the tree
        QString projName = projectionInfo[1].stripWhiteSpace();
        //      std::cout << "Examining " << shortName << std::endl; 
        if(projName.length() == 0)
        {
          // If the projection name is blank, set the node to 
          // 0 so it won't be inserted
          node = projList;
          //       std::cout << "projection name is blank: " << shortName << std::endl; 
          assert(1 == 0);
        }
        else
        {

          // Replace the underscores with blanks
          projName = projName.replace('_', ' ');
          // Get the node for this type and add the projection to it
          // If the node doesn't exist, create it
          node = lstCoordinateSystems->findItem(projName, 0);
          if(node == 0)
          {
            //          std::cout << projName << " node not found -- creating it" << std::endl;

            // the node doesn't exist -- create it
            node = new QListViewItem(projList, projName);
            //          std::cout << "Added top-level projection node: " << projName << std::endl; 
          }
        }
      }
      else
      {
        // No projection type is specified so add it to the top-level
        // projection node
        //XXX This should never happen
        node = projList;
        //      std::cout << shortName << std::endl; 
        assert(1 == 0);
      }

      // now add the coordinate system to the appropriate node

      newItem = new QListViewItem(node, srs->name());
      // display the spatial reference id in the second column of the list view

      newItem->setText(1,srs->srid());
    }
    
  } //else = proj coord sys        
#ifdef QGISDEBUG
  std::cout << "Done adding projections from spatial_ref_sys singleton" << std::endl; 
#endif  
}   


void QgsProjectionSelector::coordinateSystemSelected( QListViewItem * theItem)
{
  QgsSpatialRefSys *srs = QgsSpatialReferences::instance()->getSrsBySrid(theItem->text(1));
  if(srs)
  {

    //set the text box to show the full proection spec
#ifdef QGISDEBUG
    std::cout << "Item selected : " << theItem->text(0) << std::endl;
    std::cout << "Item selected full wkt : " << srs->srText() << std::endl;
#endif
    QString wkt = srs->srText();
    assert(wkt.length() > 0);
    // reformat the wkt to improve the display in the textedit
    // box
    wkt = wkt.replace(",", ", ");
    teProjection->setText(wkt);
    emit wktSelected(wkt);

  }
}

