/***************************************************************************
 qgsabstractoperation.h - Any operation for which progress can be monitored
--------------------------------------
Date : 23-Oct-2010
Copyright : (C) 2010 James Meyer
Email : jamesmeyerx@gmail.com
/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsabstractoperation.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSABSTRACTOPERATION_H
#define QGSABSTRACTOPERATION_H
class ANALYSIS_EXPORT QgsAbstractOperation
{
  public:
    /*! \brief Gets the current progress of this operation.
     *
     * \return The progress value between 0.0 and 1.0
     */
    virtual double progress() const = 0;
};


#endif  /* QGSABSTRACTOPERATION_H */

