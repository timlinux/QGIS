#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
settings.py - This class holds all the information for the settings dialog
of the AutoGCP plugin. This class provides no function, but rather serves as
a data struct.
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

from PyQt4 import QtCore, QtGui
#FoxHat Added - Start
from PyQt4.QtCore import *
from PyQt4.QtGui import *
#FoxHat Added - End

class AutoGcpSettings():
  
  def __init__(self):
    self.refPathChoice = 1
    self.rawPathChoice = 1
    self.refPath = ""
    self.rawPath = ""
    self.lastOpenRefPath = ""
    self.lastOpenRawPath = ""
    self.markerIconType = 1
    self.markerIconColor = 0
    self.selectedIconType = 1
    self.selectedIconColor = 1
    self.refCanvasColor = Qt.black
    self.rawCanvasColor = Qt.black

    self.loadGcpTable = 1
    self.loadRefCanvas = 1
    self.loadRawCanvas = 1
    
    self.gcpAmount = 30
    self.correlationThreshold = 0.85
    self.bandSelection = 1
    self.chipWidth = 32
    self.chipHeight = 32
    self.rmsError = 0.5
    
    self.databaseSelection = 1
    self.gcpLoading = 0
    self.sqliteChoice = 0
    self.sqlitePath = ""