/** 
    @file qgisiface.h

    $Id$
*/

#ifndef QGISIFACE_H
#define QGISIFACE_H

#include "qgisinterface.h"

/**
   @todo XXX what does this class extend in QgisInterface that makes
   it so necessary?
*/
class QgisIface : public QgisInterface
{
   public:

      QgisIface( QgisApp * qgis = 0, const char * name = 0 );

      /** @todo XXX should this be virtual? */
      ~QgisIface();

      void zoomFull();

      void zoomPrevious();

      void zoomActiveLayer();

      int getInt();

   private:

      QgisApp *qgis;

}; // class QgisIface


#endif //#define QGISIFACE_H
