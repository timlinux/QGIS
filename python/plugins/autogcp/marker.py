#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
marker.py - This class inherits from QgsMapCanvasItem and provides the same
functionality. However this class is adpated to fulfil the special needs
required by the AutoGCP plugin to add and draw GCPs to a map canvas.
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
#FoxHat Added - End

class AutoGcpMarker(QgsMapCanvasItem):
  
  def __init__(self, canvas):
    QgsMapCanvasItem.__init__(self, canvas)
    self.mCenter = QgsPoint()
    self.icon = ""
    self.s = 32
    self.x = 0
    self.y = 0
    self.width = 0
    self.height = 0
  
  def setCenter(self, point):
    self.mCenter = QgsPoint(point.x(), point.y())
    pt = QPointF()
    pt = self.toCanvasCoordinates(QgsPoint(point.x(), point.y()))
    self.setPos(pt)
  
  def updatePosition(self):
    self.setCenter(self.mCenter)
    
  def setIcon(self, iconType, iconColor): #type: 0=flag, 1=pin   color: 0=red, 1=green, 2=blue, 3=yellow
    if iconType == 0:
      self.width = 32
      self.height = 32
      self.x = -16
      self.y = -26
      if iconColor == 0:
	self.icon = ":/icons/flagred.png"
      elif iconColor == 1:
	self.icon = ":/icons/flaggreen.png"
      elif iconColor == 2:
	self.icon = ":/icons/flagblue.png"
      elif iconColor == 3:
	self.icon = ":/icons/flagyellow.png"
    elif iconType == 1:
      self.width = 32
      self.height = 32
      self.x = -4
      self.y = -30
      if iconColor == 0:
	self.icon = ":/icons/pinred.png"
      elif iconColor == 1:
	self.icon = ":/icons/pingreen.png"
      elif iconColor == 2:
	self.icon = ":/icons/pinblue.png"
      elif iconColor == 3:
	self.icon = ":/icons/pinyellow.png"
  
  def boundingRect(self):
    return QRectF(-self.s, -self.s, 2.0*self.s, 2.0*self.s)
  
  def paint(self, painter, option = 0, widget = 0): 
    pixmap = QPixmap(QSize(self.width,self.height))
    pixmap.load(self.icon)
    painter.drawPixmap(self.x, self.y, pixmap)
    self.updatePosition()
    
import resources_rc