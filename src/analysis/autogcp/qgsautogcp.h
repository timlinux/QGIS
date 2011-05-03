/***************************************************************************
 qgsautogcp.h - Library wide definitions
--------------------------------------
Date : 20-Oct-2010
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
/* $Id: qgsautogcp.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSAUTOGCP_H
#define QGSAUTOGCP_H

#if GDAL_VERSION_NUM>1720
#define AUTOGCP_THREADED
#else
#define AUTOGCP_NON_THREADED
#endif
#define BRUTE_FORCE_MATCHING
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))


#endif  /* QGSAUTOGCP_H */

