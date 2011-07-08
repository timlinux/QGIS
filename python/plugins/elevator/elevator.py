# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from elevatordialog import ElevatorDialog

class Elevator(QDialog):

  def __init__(self, iface, version = "1.0.0.0"):
    self.iface = iface
    self.version = version
    
  def getIcon(self, theName):
    myQrcPath = ":/elevator/icons/" + theName;
    if QFile.exists(myQrcPath):
      return QIcon(myQrcPath)
    else:
      return QIcon()
    pass
      
  def initGui(self):
    self.action = QAction(self.getIcon("icon.png"), "Elevator", self.iface.mainWindow())
    self.action.setWhatsThis("Extract elevation data from an online source")
    self.action.setStatusTip("Extract elevation data from an online source")
    QObject.connect(self.action, SIGNAL("triggered()"), self.run)
    self.iface.addToolBarIcon(self.action)
    self.iface.addPluginToMenu("&Elevator", self.action)
    self.dialog = ElevatorDialog(self.iface, self.version)

  def unload(self):
    self.iface.removePluginMenu("&Elevator",self.action)
    self.iface.removeToolBarIcon(self.action)

  def run(self):
    self.dialog.show()
 
