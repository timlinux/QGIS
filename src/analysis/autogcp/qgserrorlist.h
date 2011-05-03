/***************************************************************************
qgserrorlist.h - The QgsErrorList is a Singleton extension to the
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
/* $Id: qgserrorlist.h 506 2010-10-16 14:02:25Z jamesm $ */
#ifndef QGSERRORLIST_H
#define QGSERRORLIST_H

#include <QVector>
#include <QString>
#include "qgserror.h"
#include "qgslogger.h"


class QgsErrorList : public QVector<QgsError>
{
  public:
    /*! \brief Returns the valid instance
    Returns a pointer to an existing instance, or creates a new one if no other instance exists.
    */
    static QgsErrorList* instance();
    /*! \brief Report an error
    Reports an error to the main error list.
    \param The message describing the error
    \param The class name where the error occured
    \param The function name where the error occured
    \param The severity description of the error
    \param True if the error should be printed to QgsLogger::debug()
    */
    void reportError( QString message, QString classOccured, QString functionOccured, QString severity, bool printDebug );
    /*! \brief Returns the last entered error
    Returns the last submitted error in QString format
    */
    QString lastError();
    /*! \brief Deletes the last error
    */
    void deleteLastError();
    /*! \brief Composes a message
    Composes a QString formatted message based on the provided QgsError
    */
    QString composeMessage( QgsError err );
  protected:
    QgsErrorList();
    QgsErrorList( const QgsErrorList &errList ) {};
    QgsErrorList& operator=( const QgsErrorList &errList ) {};
  private:
    static QgsErrorList *onlyInstance;
};


#endif //QGSERRORLIST_H