/*
 * $Id$
 */
#ifndef QGISINTERFACE_H
#define QGISINTERFACE_H
//#include "qgisapp.h"

#include <qwidget.h>

class QgisApp;


/** interface class for plugins
    @todo XXX suggest s/QgisInterface/QgisPluginInterface/
*/
class QgisInterface : public QWidget
{
      Q_OBJECT
   public:

      QgisInterface( QgisApp * qgis = 0, const char * name = 0 );

      virtual ~QgisInterface();

public slots:

      virtual void zoomFull() = 0;
      virtual void zoomPrevious() = 0;
      virtual void zoomActiveLayer() = 0;
      virtual int getInt() = 0;

   private:

      //QgisApp *qgis;

}; // class QgisInterface

#endif //#ifndef QGISINTERFACE_H

