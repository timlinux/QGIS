/***************************************************************************
                            qgsprojectproperties.cpp
       Set various project properties (inherits qgsprojectpropertiesbase)
                              -------------------
  begin                : May 18, 2004
  copyright            : (C) 2004 by Gary E.Sherman
  email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */
#include <cassert>
#include "qgsprojectproperties.h"
#include "qgsspatialreferences.h"
#include "qgscsexception.h"

//qgis includes
#include "qgsconfig.h"
#include "qgsproject.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsrenderer.h"
#include "qgis.h"

//qt includes
#include <qcombobox.h>
#include <qfile.h>
#include <qtextedit.h>
#include <qbuttongroup.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qstring.h>
#include <qspinbox.h>
#include <qcolor.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qregexp.h>
#include <qlistview.h>
#include <qprogressdialog.h> 
#include <qapplication.h>

//stdc++ includes
#include <iostream>
#include <cstdlib>
// set the default coordinate system
static const char* defaultWktKey = "4326";
  QgsProjectProperties::QgsProjectProperties(QWidget *parent, const char *name)
: QgsProjectPropertiesBase(parent, name)
{
  //    out with the old
  //    QgsProject::instance()->mapUnits( QgsScaleCalculator::METERS );
  //    in with the new...
  QgsScaleCalculator::units myUnit = QgsProject::instance()->mapUnits();
  setMapUnits(myUnit);
  title(QgsProject::instance()->title());

  //see if the user wants on the fly projection enabled
  int myProjectionEnabledFlag = 
    QgsProject::instance()->readNumEntry("SpatialRefSys","/ProjectionsEnabled",0);
  if (myProjectionEnabledFlag==0)
  {
    cbxProjectionEnabled->setChecked(false);
  }
  else
  {
    cbxProjectionEnabled->setChecked(true);
  }
// Populate the projection list view
  getProjList();

  // If the user changes the projection for the project, we need to 
  // fire a signal to each layer telling it to change its coordinateTransform
  // member's output projection. These connects I'm setting up should be
  // automatically cleaned up when this project props dialog closes
  std::map<QString, QgsMapLayer *> myMapLayers 
    = QgsMapLayerRegistry::instance()->mapLayers();
  std::map<QString, QgsMapLayer *>::iterator myMapIterator;
  for ( myMapIterator = myMapLayers.begin(); myMapIterator != myMapLayers.end(); ++myMapIterator )
  {
    QgsMapLayer * myMapLayer = myMapIterator->second;
    connect(this,
        SIGNAL(setDestWKT(QString)),
        myMapLayer->coordinateTransform(),
        SLOT(setDestWKT(QString)));   
  }

  // get the manner in which the number of decimal places in the mouse
  // position display is set (manual or automatic)
  bool automaticPrecision = QgsProject::instance()->readBoolEntry("PositionPrecision","/Automatic");
  if (automaticPrecision)
    btnGrpPrecision->setButton(0);
  else
    btnGrpPrecision->setButton(1);

  int dp = QgsProject::instance()->readNumEntry("PositionPrecision", "/DecimalPlaces");
  spinBoxDP->setValue(dp);

  //get the snapping tolerance for digitising and set the control accordingly
  double mySnapTolerance = QgsProject::instance()->readDoubleEntry("Digitizing","/Tolerance",0);
  //leSnappingTolerance->setInputMask("000000.000000");
  leSnappingTolerance->setText(QString::number(mySnapTolerance));

  //get the line width for digitised lines and set the control accordingly
  int myLineWidth = QgsProject::instance()->readNumEntry("Digitizing","/LineWidth",0);
  spinDigitisedLineWidth->setValue(myLineWidth);    

  //get the colour of digitising lines and set the button colour accordingly
  int myRedInt = QgsProject::instance()->readNumEntry("Digitizing","/LineColorRedPart",255);
  int myGreenInt = QgsProject::instance()->readNumEntry("Digitizing","/LineColorGreenPart",0);
  int myBlueInt = QgsProject::instance()->readNumEntry("Digitizing","/LineColorBluePart",0);
  QColor myColour = QColor(myRedInt,myGreenInt,myBlueInt);
  pbnDigitisedLineColour->setPaletteBackgroundColor (myColour);

  //get the colour selections and set the button colour accordingly
  myRedInt = QgsProject::instance()->readNumEntry("Gui","/SelectionColorRedPart",255);
  myGreenInt = QgsProject::instance()->readNumEntry("Gui","/SelectionColorGreenPart",255);
  myBlueInt = QgsProject::instance()->readNumEntry("Gui","/SelectionColorBluePart",0);
  myColour = QColor(myRedInt,myGreenInt,myBlueInt);
  pbnSelectionColour->setPaletteBackgroundColor (myColour);    
}

QgsProjectProperties::~QgsProjectProperties()
{}

// return the map units
QgsScaleCalculator::units QgsProjectProperties::mapUnits() const
{
  return QgsProject::instance()->mapUnits();
}


void QgsProjectProperties::mapUnitChange(int unit)
{
  /*
     QgsProject::instance()->mapUnits(
     static_cast<QgsScaleCalculator::units>(unit));
     */
}


void QgsProjectProperties::setMapUnits(QgsScaleCalculator::units unit)
{
  // select the button
  btnGrpMapUnits->setButton(static_cast<int>(unit));
  QgsProject::instance()->mapUnits(unit);
}


QString QgsProjectProperties::title() const
{
  return titleEdit->text();
} //  QgsProjectPropertires::title() const


void QgsProjectProperties::title( QString const & title )
{
  titleEdit->setText( title );
  QgsProject::instance()->title( title );
} // QgsProjectProperties::title( QString const & title )

QString QgsProjectProperties::projectionWKT()
{
  // set the default wkt to WGS 84
  QString defaultWkt = QgsSpatialReferences::instance()->getSrsBySrid("4326")->srText();
  // the /WKT entry stores the wkt as the key into the projections map 
  QString srsWkt =  QgsProject::instance()->readEntry("SpatialRefSys","/WKT",defaultWkt);
  return srsWkt;
}  


//when user clicks apply button
void QgsProjectProperties::apply()
{
  // Set the map units
  QgsProject::instance()->mapUnits(
      static_cast<QgsScaleCalculator::units>(btnGrpMapUnits->selectedId()));

  // Set the project title
  QgsProject::instance()->title( title() );

#ifdef QGISDEBUG
  std::cout << "Projection changed, notifying all layers" << std::endl;
#endif      
  //tell the project if projections are to be used or not...      
  if (cbxProjectionEnabled->isChecked())
  {
    QgsProject::instance()->writeEntry("SpatialRefSys","/ProjectionsEnabled",1);
  }
  else
  {
    QgsProject::instance()->writeEntry("SpatialRefSys","/ProjectionsEnabled",0);
  }
  // Only change the projection if there is a node in the tree
  // selected that has an srid. This prevents error if the user
  // selects a top-level node rather than an actual coordinate
  // system
  if(lstCoordinateSystems->currentItem()->text(1).length() > 0)
  {
    //notify all layers the output projection has changed

          QString selectedWkt = QgsSpatialReferences::instance()->getSrsBySrid(lstCoordinateSystems->currentItem()->text(1))->srText();
    emit setDestWKT(selectedWkt);   //update the project props
    // write the projection's wkt to the project settings rather
    QgsProject::instance()->writeEntry("SpatialRefSys","/WKT",
        lstCoordinateSystems->currentItem()->text(0));
  }

  // set the mouse display precision method and the
  // number of decimal places for the manual option
  if (btnGrpPrecision->selectedId() == 0)
    QgsProject::instance()->writeEntry("PositionPrecision","/Automatic", true);
  else
    QgsProject::instance()->writeEntry("PositionPrecision","/Automatic", false);
  QgsProject::instance()->writeEntry("PositionPrecision","/DecimalPlaces", spinBoxDP->value());
  // Announce that we may have a new display precision setting
  emit displayPrecisionChanged();

  //set the snapping tolerance for digitising (we write as text but read will convert to a num
  QgsProject::instance()->writeEntry("Digitizing","/Tolerance",leSnappingTolerance->text());

  //set the line width for digitised lines and set the control accordingly
  QgsProject::instance()->writeEntry("Digitizing","/LineWidth",spinDigitisedLineWidth->value());

  //set the colour of digitising lines
  QColor myColour = pbnDigitisedLineColour->paletteBackgroundColor();
  QgsProject::instance()->writeEntry("Digitizing","/LineColorRedPart",myColour.red());
  QgsProject::instance()->writeEntry("Digitizing","/LineColorGreenPart",myColour.green());
  QgsProject::instance()->writeEntry("Digitizing","/LineColorBluePart",myColour.blue());

  //set the colour for selections
  myColour = pbnSelectionColour->paletteBackgroundColor();
  QgsProject::instance()->writeEntry("Gui","/SelectionColorRedPart",myColour.red());
  QgsProject::instance()->writeEntry("Gui","/SelectionColorGreenPart",myColour.green());
  QgsProject::instance()->writeEntry("Gui","/SelectionColorBluePart",myColour.blue()); 
  QgsRenderer::mSelectionColor=myColour;

}

//when user clicks ok
void QgsProjectProperties::accept()
{
  apply();
  close();
}
void QgsProjectProperties::getProjList()
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

  // XXX THIS IS BROKEN AND DISABLED
  // Read the users custom coordinate system (CS) file
  // Get the user home dir. On Unix, this is $HOME. On Windows and MacOSX we
  // will use the application directory.
  //
  // Note that the global cs file must exist or the user file will never
  // be read.
  //
  // XXX Check to make sure this works on OSX

  // construct the path to the users custom CS file
  /*
#if defined(WIN32) || defined(Q_OS_MACX)
customCsFile = PKGDATAPATH + "/share/qgis/user_defined_cs.txt";
#else

customCsFile = getenv("HOME");
customCsFile += "/.qgis/user_defined_cs.txt";
#endif
QFile csQFile( customCsFile );
if ( csQFile.open( IO_ReadOnly ) ) 
{
QTextStream userCsTextStream( &csQFile );

  // Read the user-supplied CS file 
  while ( !userCsTextStream.atEnd() ) 
  {
  myCurrentLineQString = userCsTextStream.readLine(); // line of text excluding '\n'
  //get the user friendly name for the WKT
  QString myShortName = getWKTShortName(myCurrentLineQString);
  //XXX Fix this so it can read user defined projections - 
  //mProjectionsMap[myShortName]=myCurrentLineQString;
  }
  csQFile.close();
  }

  // end of processing users custom CS file
  */
  // Determine the current project projection so we can select the 
  // correct entry in the combo
  QString currentSrid = QgsProject::instance()->readEntry("SpatialRefSys","/WKT",defaultWktKey);
  QListViewItem * mySelectedItem = 0;
  // get the reference to the map
  projectionWKTMap_t mProjectionsMap = srs->getMap();
  // get an iterator for the map
  projectionWKTMap_t::iterator myIterator;
  //find out how many entries in the map
  int myEntriesCount = mProjectionsMap.size();
  std::cout << "Srs map has " << myEntriesCount << " entries " << std::endl;
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
    if (myIterator.key()==currentSrid)
    {
      // this is the selected item -- store it for future use
      mySelectedItem = newItem;
    }
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
    if (myIterator.key()==currentSrid)
      mySelectedItem = newItem;
  }
} //else = proj coord sys        


/**
//make sure all the loaded layer WKT's and the active project projection exist in the 
//combo box too....
std::map<QString, QgsMapLayer *> myMapLayers = QgsMapLayerRegistry::instance()->mapLayers();
std::map<QString, QgsMapLayer *>::iterator myMapIterator;
for ( myMapIterator = myMapLayers.begin(); myMapIterator != myMapLayers.end(); ++myMapIterator )
{
QgsMapLayer * myMapLayer = myMapIterator->second;
QString myWKT = myMapLayer->getProjectionWKT();
QString myWKTShortName = getWKTShortName(myWKT);
//TODO add check here that CS is not already in the projections map
//and if not append to wkt_defs file
cboProjection->insertItem(myIterator.key());
mProjectionsMap[myWKTShortName]=myWKT;
}    

//set the combo entry to the current entry for the project
cboProjection->setCurrentText(mySelectedKey);
*/
lstCoordinateSystems->setCurrentItem(mySelectedItem);
lstCoordinateSystems->ensureItemVisible(mySelectedItem);
/*
   for ( myIterator = mProjectionsMap.begin(); myIterator != mProjectionsMap.end(); ++myIterator ) 
   {
//std::cout << "Widget map has: " <<myIterator.key().ascii() << std::endl;
//cboProjection->insertItem(myIterator.key());
if(myIterator.key().find("Lat/Long") > -1)
{
new QListViewItem(geoList, myIterator.key());
}
else
{
new QListViewItem(projList, myIterator.key());
}
}

*/
std::cout << "Done adding projections from spatial_ref_sys singleton" << std::endl; 
}   


void QgsProjectProperties::coordinateSystemSelected( QListViewItem * theItem)
{
  QgsSpatialRefSys *srs = QgsSpatialReferences::instance()->getSrsBySrid(theItem->text(1));
  if(srs)
  {

    //set the text box to show the full proection spec
    std::cout << "Item selected : " << theItem->text(0) << std::endl;
    std::cout << "Item selected full wkt : " << srs->srText() << std::endl;
    QString wkt = srs->srText();
    assert(wkt.length() > 0);
    // reformat the wkt to improve the display in the textedit
    // box
    wkt = wkt.replace(",", ", ");
    teProjection->setText(wkt);
  }
}
QString QgsProjectProperties::getWKTShortName(QString theWKT)
{
  if (!theWKT) return NULL;
  if (theWKT.isEmpty()) return NULL;
  /* for example 
     PROJCS["Kertau / Singapore Grid",GEOGCS["Kertau",DATUM["Kertau",SPHEROID["Everest 1830 Modified",6377304.063,300.8017]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]],PROJECTION["Cassini_Soldner"],PARAMETER["latitude_of_origin",1.28764666666667],PARAMETER["central_meridian",103.853002222222],PARAMETER["false_easting",30000],PARAMETER["false_northing",30000],UNIT["metre",1]]

     We want to pull out 
     Kertau / Singapore Grid
     and
     Cassini_Soldner
     */
  OGRSpatialReference mySpatialRefSys;
  //this is really ugly but we need to get a QString to a char**
  char * mySourceCharArrayPointer = (char*) theWKT.ascii();

  /* Here are the possible OGR error codes :
     typedef int OGRErr;

#define OGRERR_NONE                0
#define OGRERR_NOT_ENOUGH_DATA     1    --> not enough data to deserialize 
#define OGRERR_NOT_ENOUGH_MEMORY   2
#define OGRERR_UNSUPPORTED_GEOMETRY_TYPE 3
#define OGRERR_UNSUPPORTED_OPERATION 4
#define OGRERR_CORRUPT_DATA        5
#define OGRERR_FAILURE             6
#define OGRERR_UNSUPPORTED_SRS     7 */

  OGRErr myInputResult = mySpatialRefSys.importFromWkt( & mySourceCharArrayPointer );
  if (myInputResult != OGRERR_NONE)
  {
    return NULL;
  }
  //std::cout << theWKT << std::endl;
  //check if the coordinate system is projected or not

  // if the spatial ref sys starts with GEOGCS, the coordinate
  // system is not projected
  QString myProjection,myDatum,myCoordinateSystem,myName;
  if(theWKT.find(QRegExp("^GEOGCS")) == 0)
  {
    myProjection = "Lat/Long";
    myCoordinateSystem = mySpatialRefSys.GetAttrValue("GEOGCS",0);
    myName = myProjection + " - " + myCoordinateSystem.replace('_', ' ');
  }  
  else
  {    

    myProjection = mySpatialRefSys.GetAttrValue("PROJCS",0);
    myCoordinateSystem = mySpatialRefSys.GetAttrValue("PROJECTION",0);
    myName = myProjection + " - " + myCoordinateSystem.replace('_', ' ');
  } 
  //std::cout << "Projection short name " << myName << std::endl;
  return myName; 
}
