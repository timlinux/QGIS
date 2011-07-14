#!python
# -*- coding: utf-8 -*-
"""
***************************************************************************************
coordinatereader.py - The class for reading the dim files
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

from PyQt4 import QtCore, QtXml
from PyQt4.QtCore import *
from PyQt4.QtXml import *
from qgis.core import *

class CoordinateReader:

  def __init__(self):
    self.mTopLeftX = 0.0
    self.mTopLeftY = 0.0
    self.mTopRightX = 0.0
    self.mTopRightY = 0.0
    self.mBottomRightX = 0.0
    self.mBottomRightY = 0.0
    self.mBottomLeftX = 0.0
    self.mBottomLeftY = 0.0
    self.mRows = 0
    self.mColumns = 0
    self.mEpsg = ""
    
  def topLeftX(self):
    return self.mTopLeftX
    
  def topLeftY(self):
    return self.mTopLeftY
    
  def topRightX(self):
    return self.mTopRightX
    
  def topRightY(self):
    return self.mTopRightY
    
  def bottomLeftX(self):
    return self.mBottomLeftX
    
  def bottomLeftY(self):
    return self.mBottomLeftY
    
  def bottomRightX(self):
    return self.mBottomRightX
    
  def bottomRightY(self):
    return self.mBottomRightY
    
  def rows(self):
    return self.mRows
    
  def columns(self):
    return self.mColumns
    
  def epsg(self):
    return self.mEpsg
    
  def readCoordinates(self, path):
    doc = QDomDocument( "DIM" );
    xmlFile = QFile( path )    
    if not xmlFile.exists() or not xmlFile.open( QIODevice.ReadOnly ):
      return False
    if not doc.setContent( xmlFile ):
      xmlFile.close()
      return False
    xmlFile.close()
    docElement = doc.documentElement()
    frame = docElement.elementsByTagName("Dataset_Frame").item(0)
    vertex1 = frame.childNodes().item(0)
    x1 = float(vertex1.firstChildElement("FRAME_LON").text())
    y1 = float(vertex1.firstChildElement("FRAME_LAT").text())
    vertex2 = frame.childNodes().item(1)
    x2 = float(vertex2.firstChildElement("FRAME_LON").text())
    y2 = float(vertex2.firstChildElement("FRAME_LAT").text())
    vertex3 = frame.childNodes().item(2)
    x3 = float(vertex3.firstChildElement("FRAME_LON").text())
    y3 = float(vertex3.firstChildElement("FRAME_LAT").text())
    vertex4 = frame.childNodes().item(3)
    x4 = float(vertex4.firstChildElement("FRAME_LON").text())
    y4 = float(vertex4.firstChildElement("FRAME_LAT").text())
    x = [x1, x2, x3, x4]
    y = [y1, y2, y3, y4]
    
    smallX = x1
    smallY = y1
    bigX = x1
    bigY = y1
    for xVal in x:
      if xVal < smallX:
	smallX = xVal
      if xVal > bigX:
	bigX = xVal
    for yVal in y:
      if yVal < smallY:
	smallY = yVal
      if yVal > bigY:
	bigY = yVal
	
    avgX = (smallX + bigX) / 2
    avgY = (smallY + bigY) / 2
    
    for i in range(len(x)):
      xVal = x[i]
      yVal = y[i]
      if xVal < avgX and yVal > avgY:
	self.mTopLeftX = xVal
	self.mTopLeftY = yVal
      elif xVal > avgX and yVal > avgY:
	self.mTopRightX = xVal
	self.mTopRightY = yVal
      elif xVal > avgX and yVal < avgY:
	self.mBottomRightX = xVal
	self.mBottomRightY = yVal
      elif xVal < avgX and yVal < avgY:
	self.mBottomLeftX = xVal
	self.mBottomLeftY = yVal
	
    """self.mTopLeftX = x1
    self.mTopLeftY = y1
    self.mTopRightX = x2
    self.mTopRightY = y2
    self.mBottomRightX = x3
    self.mBottomRightY = y3
    self.mBottomLeftX = x4
    self.mBottomLeftY = y4"""
	
    
    self.mRows = int(vertex3.firstChildElement("FRAME_ROW").text())
    self.mColumns = int(vertex3.firstChildElement("FRAME_COL").text())
    referenceSystem = docElement.elementsByTagName("Coordinate_Reference_System").item(0)
    self.mEpsg = referenceSystem.childNodes().item(1).firstChildElement("HORIZONTAL_CS_CODE").text()
    return True