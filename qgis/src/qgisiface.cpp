/**
   @file qgisiface.cpp
*/

#include "qgisiface.h"

#include "qgisapp.h"



static const char * const ident_ = "$Id$";




// XXX ummm, why isn't "name" used?
QgisIface::QgisIface( QgisApp * _qgis, const char * name ) 
   : qgis(_qgis) 
{}



QgisIface::~QgisIface()
{}


void
QgisIface::zoomFull()
{
   qgis->zoomFull();
}


void
QgisIface::zoomPrevious()
{
   qgis->zoomPrevious();
}


void
QgisIface::zoomActiveLayer()
{
   qgis->zoomToLayerExtent();
}


int
QgisIface::getInt()
{
   return qgis->getInt();
}
