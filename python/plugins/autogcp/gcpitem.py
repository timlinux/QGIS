#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
gcpitem.py - The class holds all the information and objects required to
draw markers representing GCPs on the map canvas of the AutoGCP plugin.
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
from qgis.core import *
from qgis.gui import *
from marker import *
#FoxHat Added - End

class AutoGcpItem():
  
  def __init__(self, refCanvas, rawCanvas):
    self.refCenter = AutoGcpPoint()
    self.rawCenter = AutoGcpPoint()
    self.refMarker = AutoGcpMarker(refCanvas)
    self.rawMarker = AutoGcpMarker(rawCanvas)
    self.row = AutoGcpRow()
    self.gotRawGcp = False


  def getRefCenter(self):
    return self.refCenter

  def getRawCenter(self):
    return self.rawCenter

  def getRefMarker(self):
    return self.refMarker

  def getRawMarker(self):
    return self.rawMarker

  def getId(self):
    return self.row.getId()

  def getSrcX(self):
    return self.getSrcX()

  def getSrcY(self):
    return self.row.getSrcY()

  def getDstX(self):
    return self.row.getDstX()

  def getDstY(self):
    return self.row.getDstY()

  def getMatch(self):
    return self.row.getMatch()

  def getShow(self):
    return self.row.getShow()

  def getRow(self):
    return self.row

  def getRefCanvasPoint(self):
    return self.refMarker.toCanvasCoordinates(self.refCenter)

  def getRawCanvasPoint(self):
    return self.rawMarker.toCanvasCoordinates(self.rawCenter)

  def hasRaw(self):
    return self.gotRawGcp


  def setRefCenter(self, x, y):
    self.setRefCenter(QgsPoint(x, y))

  def setRefCenter(self, newVal):
    self.refCenter = newVal
    self.refMarker.setCenter(self.refCenter)
    self.setSrcX(self.refCenter.x())
    self.setSrcY(self.refCenter.y())

  def setRawCenter(self, x, y):
    self.setRawCenter(QgsPoint(x, y))

  def setRawCenter(self, newVal):
    self.rawCenter = newVal
    self.rawMarker.setCenter(self.rawCenter)
    self.gotRawGcp = True
    self.setDstX(self.rawCenter.x())
    self.setDstY(self.rawCenter.y())

  def setId(self, newVal):
    self.row.setId(newVal)

  def setSrcX(self, newVal):
    self.row.setSrcX(newVal)

  def setSrcY(self, newVal):
    self.row.setSrcY(newVal)

  def setDstX(self, newVal):
    self.row.setDstX(newVal)

  def setDstY(self, newVal):
    self.row.setDstY(newVal)

  def setMatch(self, newVal):
    self.row.setMatch(newVal)

  def setShow(self, newVal):
    self.row.setShow(newVal)

  def setRow(self, newVal):
    self.row = newVal



class AutoGcpRow():

  def __init__(self, i = -1, s1 = -1, s2 = -1, d1 = -1, d2 = -1, m = 0, s = 1):
    self.id = i
    self.srcX = s1
    self.srcY = s2
    self.dstX = d1
    self.dstY = d2
    self.match = m
    self.show = s

  def getId(self):
    return self.id

  def getSrcX(self):
    return self.srcX

  def getSrcY(self):
    return self.srcY

  def getDstX(self):
    return self.dstX

  def getDstY(self):
    return self.dstY

  def getMatch(self):
    return self.match

  def getShow(self):
    return self.show

  def getIdItem(self):
    return AutoGcpTableRow("gcp_"+str(self.id), QTableWidgetItem.Type)

  def getSrcXItem(self):
    return AutoGcpTableRow(str(self.srcX), QTableWidgetItem.Type)

  def getSrcYItem(self):
    return AutoGcpTableRow(str(self.srcY), QTableWidgetItem.Type)

  def getDstXItem(self):
    if self.dstX == -1:
      return AutoGcpTableRow("--", QTableWidgetItem.Type)
    else:
      return AutoGcpTableRow(str(self.dstX), QTableWidgetItem.Type)

  def getDstYItem(self):
    if self.dstY == -1:
      return AutoGcpTableRow("--", QTableWidgetItem.Type)
    else:
      return AutoGcpTableRow(str(self.dstY), QTableWidgetItem.Type)

  def getMatchItem(self):
    bar = QProgressBar()
    bar.setMinimum(0)
    bar.setMaximum(100)
    bar.setValue(self.match)
    bar.setTextVisible(True)
    return bar

  def getShowItem(self):
    gcpItemShow = AutoGcpTableRow("", QTableWidgetItem.Type)
    if self.show == 0:
      gcpItemShow.setCheckState(0)
    else:
      gcpItemShow.setCheckState(2)
    return gcpItemShow

  def setId(self, newVal):
    self.id = newVal

  def setSrcX(self, newVal):
    self.srcX = newVal

  def setSrcY(self, newVal):
    self.srcY = newVal

  def setDstX(self, newVal):
    self.dstX = newVal

  def setDstY(self, newVal):
    self.dstY = newVal

  def setMatch(self, newVal):
    self.match = newVal

  def setShow(self, newVal):
    self.show = newVal


class AutoGcpTableRow(QTableWidgetItem):

  def __init__(self):
    super(AutoGcpTableRow, self).__init__()

  def __init__(self, x1, x2):
    super(AutoGcpTableRow, self).__init__(x1, x2)



class AutoGcpPoint(QgsPoint):

  def __init__(self):
    super(AutoGcpPoint, self).__init__()