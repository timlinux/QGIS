/***************************************************************************
qgserrort.h - The QgsError class is used to describe an error that occured
during some stage of execution.  Information about an error is captured here
and then stored in a QgsErrorList.
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
/* $Id: qgserror.h 606 2010-10-29 09:13:39Z jamesm $ */
#ifndef QGSERROR_H
#define QGSERROR_H

#include <QString>
#include <QList>

class QgsError
{
  public:
    QgsError()
        : mMessage( "" ), mClassOccured( "" ), mFunctionOccured( "" ), mErrorSeverity( QString::number( 0 ) ), mViewed( FALSE ) {};

    ~QgsError()
    {};

    void setMessage( QString message )
    { mMessage = message; };

    void setClassOccured( QString classOccured )
    { mClassOccured = classOccured; };

    void setFunctionOccured( QString funcOccured )
    { mFunctionOccured = funcOccured; };

    void setSeverity( QString severity )
    { mErrorSeverity = severity; };

    void setViewed( bool viewed )
    { mViewed = viewed; };

    QString message()
    { return mMessage; };

    QString classOccured()
    { return mClassOccured; };

    QString functionOccured()
    { return mFunctionOccured; };

    QString severity()
    { return mErrorSeverity; };

    bool viewed()
    { return mViewed; };

  private:
    QString mMessage;
    QString mClassOccured;
    QString mFunctionOccured;
    QString mErrorSeverity;
    bool mViewed;
};


#endif //QGSERROR_H