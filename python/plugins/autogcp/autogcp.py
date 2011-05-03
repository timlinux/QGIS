#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
autogcp.py - The class to handle the basic functionality required by QGis.
The graphical user interface will be constructed and destructed using this
class.
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

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from gui import *
import resources_rc

class AutoGCPPlugin:
 def __init__(self, iface):
   # save reference to the QGIS interface
   self.iface = iface
    
 def getIcon(self, theName):
    # get the icon
    myQrcPath = ":/icons/" + theName;
    if QFile.exists(myQrcPath):
      return QIcon(myQrcPath)
    else:
      return QIcon()
      
 def initGui(self):#This method is mandatory for a Qgis Plugin	
   # create action that will start plugin configuration
   self.action = QAction(self.getIcon("autogcp.png"), "Auto GCP Extraction", self.iface.mainWindow())
   self.action.setWhatsThis("Automatic GCP extraction, cross-correlation and orthorectification")
   self.action.setStatusTip("Automatic GCP extraction, cross-correlation and orthorectification")
   QObject.connect(self.action, SIGNAL("triggered()"), self.run)
   # add toolbar button and menu item
   self.iface.addToolBarIcon(self.action)
   self.iface.addPluginToMenu("&Auto GCPs", self.action)
   self.window = MainWindow(self.iface)

 def unload(self): #This method is mandatory for a Qgis plugin
   # remove the plugin menu item and icon
   self.iface.removePluginMenu("&Auto GCP Extraction",self.action)
   self.iface.removeToolBarIcon(self.action)

 def run(self):
   # create and show a configuration dialog or something similar
   screen = QtGui.QApplication.desktop();
   self.window.resize(screen.width(), screen.height())
   self.window.show()
 


