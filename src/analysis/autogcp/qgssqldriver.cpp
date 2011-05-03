/***************************************************************************
qgssqlitedriver.cpp - The abstract driver resposible for the SQL database
connection
--------------------------------------
Date : 02-July-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgssqldriver.cpp 506 2010-10-16 14:02:25Z jamesm $ */
#include "qgssqldriver.h"

bool QgsSqlDriver::isOpen()
{
  return mIsOpen;
}

void QgsSqlDriver::setHashAlgorithm( QgsHasher::HashAlgorithm hashAlgorithm )
{
  mHashAlgorithm = hashAlgorithm;
}

QgsHasher::HashAlgorithm QgsSqlDriver::hashAlgorithm()
{
  return mHashAlgorithm;
}

QgsSqlDriver::~QgsSqlDriver()
{

}
