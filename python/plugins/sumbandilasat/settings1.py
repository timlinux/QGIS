# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from qgis.sumbandilasat import *

class Settings():

  def __init__(self, ui):
    self.settings = QSettings("QuantumGIS", "SumbandilaSat Image Processor")
    #No settings exist
    if self.settings.value("init").toInt() == 0:
      self.createSettings(ui)
    else:
      loadSettings(ui)
  
  def loadSettings(self, ui):
    pass
  
  def createSettings(self, ui):
    pass
  
  def columnGainThreshold(self):
    pass
    
  def columnBiasThreshold(self):
    pass