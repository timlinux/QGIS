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

#include "qgsprojectionselectorbase.h"



/**
@author Tim Sutton
*/
class QgsProjectionSelector: public QgsProjectionSelectorBase
{
Q_OBJECT
public:
  //XXX made flags optional
    QgsProjectionSelector( QWidget* parent , const char* name ,WFlags fl =0  );
    ~QgsProjectionSelector();
    
  
private:
    
signals:
};

#endif
