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

//qt includes
#include <qlistview.h>
#include <qtextedit.h>

//standard includes
#include <iostream>


QgsProjectionSelector::QgsProjectionSelector( QWidget* parent , const char* name , WFlags fl  )
  : QgsProjectionSelectorBase( parent, name, fl )
{
}

QgsProjectionSelector::~QgsProjectionSelector()
{
}

