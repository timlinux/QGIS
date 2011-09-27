/***************************************************************************
                              qgsdarwincoreplugin.cpp
                              -----------------------
  begin                : September 24, 2010
  copyright            : (C) 2010 by Marco Hugentobler
  email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdarwincoreplugin.h"
#include "qgsdarwincoredialog.h"
#include "qgis.h"
#include "qgisinterface.h"
#include <QAction>
#include <QFileInfo>
#include <QToolBar>

static const QString name_ = QObject::tr( "Darwin core plugin" );
static const QString description_ = QObject::tr( "Adds darwin core layers" );
static const QString version_ = QObject::tr( "Version 0.1" );

QgsDarwinCorePlugin::QgsDarwinCorePlugin( QgisInterface* iface ): mIface( iface ), mAction( 0 )
{

}

QgsDarwinCorePlugin::~QgsDarwinCorePlugin()
{
}

void QgsDarwinCorePlugin::initGui()
{
  if( mIface )
  {
    mAction = new QAction( QIcon(":/darwincore.gif"), tr("Open darwin core source"), this);
    QObject::connect( mAction, SIGNAL( triggered() ), this, SLOT(showDatasourceDialog() ) );
    mIface->layerToolBar()->addAction( mAction );
  }
}

void QgsDarwinCorePlugin::unload()
{
  if( mIface )
  {
    mIface->removeToolBarIcon( mAction );
    delete mAction;
    mAction = 0;
  }
}

void QgsDarwinCorePlugin::showDatasourceDialog()
{
  QgsDarwinCoreDialog d;
  if( d.exec() == QDialog::Accepted )
  {
    QString ds = d.datasource();
    mIface->addVectorLayer( ds, QFileInfo( ds ).baseName(), "darwin_core");
  }
}

QGISEXTERN QgisPlugin * classFactory( QgisInterface * theQgisInterfacePointer )
{
  return new QgsDarwinCorePlugin( theQgisInterfacePointer );
}

QGISEXTERN QString name()
{
  return name_;
}

QGISEXTERN QString description()
{
  return description_;
}

QGISEXTERN QString version()
{
  return version_;
}

QGISEXTERN int type()
{
  return QgisPlugin::UI;
}

QGISEXTERN void unload( QgisPlugin* thePluginPointer )
{
  delete thePluginPointer;
}
