# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from leechydialog import LeechyDialog

class Leechy(QDialog):

  def __init__(self, iface, version = "1.0.0.0"):
    self.iface = iface
    self.version = version
    
  def getIcon(self, theName):
    myQrcPath = ":/leechy/icons/" + theName;
    if QFile.exists(myQrcPath):
      return QIcon(myQrcPath)
    else:
      return QIcon()
    pass
      
  def initGui(self):
    self.action = QAction(self.getIcon("icon.png"), "Leechy", self.iface.mainWindow())
    self.action.setWhatsThis("Extract images from various online sources such as Google and Yahoo")
    self.action.setStatusTip("Extract images from various online sources such as Google and Yahoo")
    QObject.connect(self.action, SIGNAL("triggered()"), self.run)
    self.iface.addToolBarIcon(self.action)
    self.iface.addPluginToMenu("&Leechy", self.action)
    self.dialog = LeechyDialog(self.iface, self.version)

  def unload(self):
    self.iface.removePluginMenu("&Leechy",self.action)
    self.iface.removeToolBarIcon(self.action)

  def run(self):
    self.dialog.show()
 
