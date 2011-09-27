/***************************************************************************
                              qgsdarwincoreplugin.h
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

#ifndef QGSDARWINCOREPLUGIN_H
#define QGSDARWINCOREPLUGIN_H

#include "qgisplugin.h"
#include <QObject>

class QAction;
class QgisInterface;

/**A plugin to load data with the darwin core provider (from local file or url)*/
class QgsDarwinCorePlugin: public QObject, public QgisPlugin
{
  Q_OBJECT
  public:
    QgsDarwinCorePlugin( QgisInterface* iface );
    ~QgsDarwinCorePlugin();

    /**initialize connection to GUI*/
    void initGui();
    /**Unload the plugin and cleanup the GUI*/
    void unload();

  private:
    QgisInterface* mIface;
    QAction* mAction;

  private slots:
    void showDatasourceDialog();
};

#endif // QGSDARWINCOREPLUGIN_H
