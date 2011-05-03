/***************************************************************************
qgserrorlist.cpp - The QgsErrorList is a Singleton extension to the
QVector class that provides container functionality which is used to capture
and store any exceptions that may occur in other classes.
This allows errors or exceptions to be stored with extensive detail for use
by other classes or debugging purposes.
--------------------------------------
Date : 12-July-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
Author: Francois Maass
/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgserrorlist.cpp 506 2010-10-16 14:02:25Z jamesm $ */
#include "qgserrorlist.h"

QgsErrorList* QgsErrorList::onlyInstance = 0;

QgsErrorList::QgsErrorList() : QVector<QgsError>()
{

}

QgsErrorList* QgsErrorList::instance()
{
  if ( onlyInstance == 0 )
    onlyInstance = new QgsErrorList;
  return onlyInstance;

}

void QgsErrorList::reportError( QString message, QString classOccured, QString functionOccured, QString severity, bool printDebug )
{
  QgsError err;
  err.setMessage( message );
  err.setClassOccured( classOccured );
  err.setFunctionOccured( functionOccured );
  err.setSeverity( severity );
  QVector<QgsError>::append( err );

  if ( printDebug )
  {
    QString msg = classOccured + ": " + severity + " error. " + message;
    QgsLogger::debug( msg.toLatin1().data() );
  }
}

QString QgsErrorList::lastError()
{
  QgsError &err = QVector<QgsError>::last();
  return composeMessage( err );
}

void QgsErrorList::deleteLastError()
{
  QVector<QgsError>::erase( QVector<QgsError>::end() - 1 );
}

QString QgsErrorList::composeMessage( QgsError err )
{
  QString tmp( "Error: \n" );
  tmp += "Severity: " + err.severity() + "\n";
  tmp += "Message: " + err.message() + "\n";
  tmp += "Occured in Class: " + err.classOccured() + "\n";
  tmp += "Occured in Function: " + err.functionOccured() + "\n";
  return tmp;
}
