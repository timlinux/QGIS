#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
__init__.py - The initilaztion definition for the AutoGCP Python plugin.
--------------------------------------
Date : 21-September-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
Modified: Christoph Stallmann
Modification Date: 21/09/2010
/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
"""

def name():
 return "AutoGCP Plugin"
def description():
 return "Performs automated extraction, cross-referencing and orthorectification of images"
def version():
 return "Version 0.5"
def qgisMinimumVersion(): 
 return "1.0"
def authorName():
 return "FoxHat Solutions"
def classFactory(iface):
 #This loads the plugin with the supplied QGIS interface
 from autogcp import AutoGCPPlugin
 return AutoGCPPlugin(iface)

