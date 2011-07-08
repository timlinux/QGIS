#!python
# -*- coding: utf-8 -*-
"""
***************************************************************************************
__init__.py - The initilaztion definition for the SumbandilaSat Level 3 System plug-in.
--------------------------------------
Date : 21-January-2011
Email : christoph.stallmann<at>gmail.com
Created by: Christoph Stallmann
Last modified by: Christoph Stallmann
Last modification date: 21-January-2011
***************************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************************
"""

def name():
	return "SumbandilaSat Level 3 Processing System"

def description():
	return "Tools for creating level 3 products for SumbandilaSat"

def version():
	return "0.0.0.1"
  
def qgisMinimumVersion():
	return "1.6"

def icon():
	return "icons/icon.png"
	
def authorName():
	return "Christoph Frank Stallmann"

def classFactory( iface ):
	from sumbandilasat import sumbandilasatPlugin
	return sumbandilasatPlugin( iface )
