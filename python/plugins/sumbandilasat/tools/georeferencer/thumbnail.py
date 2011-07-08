#!python
# -*- coding: utf-8 -*-
"""
***************************************************************************************
thumbnail.py - The class containing the thumbnail information
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

from PyQt4 import QtCore
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from coordinatereader import CoordinateReader
from worldfiler import WorldFiler
import math
from math import *
import shutil
import os.path

class Thumbnail:

  def __init__(self, imagePath, refcrs, sourcePath = "", flip = False):
    self.mImagePath = imagePath
    self.mLayerId = ""
    self.refcrs = refcrs
    self.invalidCoordinates = False
    
    self.TL = QgsPoint()
    self.TR = QgsPoint()
    self.BR = QgsPoint()
    self.BL = QgsPoint()
    
    self.newTL = QgsPoint()
    self.newTR = QgsPoint()
    self.newBR = QgsPoint()
    self.newBL = QgsPoint()

    self.mRows = 0
    self.mColumns = 0
    
    self.flip = flip
    
    self.pixelX = 0.0
    self.pixelY = 0.0
    self.rotationX = 0.0
    self.rotationY = 0.0
    
    self.mXOffset = 0.0
    self.mYOffset = 0.0
    self.mEpsg = "EPSG:4326"
    
    self.mGdalDatasetPath = ""
    self.mGdalDatasetLayer = None
    self.mFinishedRendering = False
    
    self.hasWorldSource = False
    self.hasDimSource = False
    self.tifPath = ""
    self.mSourcePath = QString(sourcePath)
    
    self.wasGeoreferenced = False
    self.factor = 0
      
    if sourcePath == "AUTO":
      self.mSourcePath = QString("../16bit/*.dim")
      self.flip = True
      
    if self.mSourcePath.endsWith(".dim"):
      myDir = QDir( QFileInfo( imagePath ).absoluteDir() )
      myDir.cd(self.mSourcePath.left(self.mSourcePath.length()-5))
      info = QFileInfo( imagePath )
      name = QString( info.fileName() )
      stop = name.lastIndexOf( "_" )
      fileList = myDir.entryInfoList("*.dim")
      for theFile in fileList:
	if theFile.fileName().contains(info.fileName().left(stop)) and theFile.absoluteFilePath().endsWith(".dim"):
	  self.mSourcePath = theFile.absoluteFilePath()
	  self.tifPath = self.mSourcePath.left(self.mSourcePath.length()-4)+".tif"
	  break
      if not self.mSourcePath == "":
	self.hasDimSource = self.readCoordinates( self.mSourcePath )
      if self.hasDimSource:
	self.createOriginalAttributes()
	
    if not self.hasDimSource and sourcePath == "AUTO": #If dim AUTO failed, try it with world file
      self.mSourcePath = QString("./*.jgw")
      self.flip = False
      self.mEpsg = "EPSG:4326"
      
    if not self.hasDimSource and self.mSourcePath.endsWith(".jgw"):
      myDir = QDir( QFileInfo( self.mImagePath ).absoluteDir() )
      myDir.cd(self.mSourcePath.left(self.mSourcePath.length()-5))
      info = QFileInfo( self.mImagePath )
      name = QString( info.fileName() )
      stop = name.lastIndexOf( "_" )
      fileList = myDir.entryInfoList("*.jgw")
      for theFile in fileList:
	if theFile.fileName().contains(info.fileName().left(stop)) and theFile.absoluteFilePath().endsWith(".jgw"):
	  self.mSourcePath = theFile.absoluteFilePath()
	  break
    
  #Convert to google coordinate system, if jgw file is not in google coordinate format
  def convertCoordinates(self):
    if not self.hasDimSource and self.mSourcePath.endsWith(".jgw"):
      if self.mSourcePath != "":
	self.readJpegDimension()
	self.readWorldFileData()
	self.hasWorldSource = True
	return self.createOriginalAttributes()
    return False
    
  def newTopLeftX(self):
    return self.topLeftXNew
  
  def newTopLeftY(self):
    return self.topLeftYNew
    
  def newTopRightX(self):
    return self.topRightXNew
    
  def newTopRightY(self):
    return self.topRightYNew
    
  def newBottomRightX(self):
    return self.bottomRightXNew
    
  def newBottomRightY(self):
    return self.bottomRightYNew
    
  def newBottomLeftX(self):
    return self.bottomLeftXNew
         
  def newBottomLeftY(self):
    return self.bottomLeftYNew
    
  def isFinishedRendering(self):
    return self.mFinishedRendering
    
  def setCorners(self, a, b, c, d):
    self.TL = QgsPoint(a)
    self.TR = QgsPoint(b)
    self.BR = QgsPoint(c)
    self.BL = QgsPoint(d)
    
  def setSize(self, width, height):
    self.mRows = height
    self.mColumns = width
    
  def hasSourceFile(self):
    return self.hasDimSource or self.hasWorldSource
    
  def imagePath(self):
    return self.mImagePath
    
  def sourcePath(self):
    return self.mSourcePath
    
  def id(self):
    return self.mLayerId
    
  def rows(self):
    return self.mRows
    
  def columns(self):
    return self.mColumns
    
  def layer(self):
    return self.mGdalDatasetLayer
    
  def xOffset(self):
    return self.mXOffset
    
  def yOffset(self):
    return self.mYOffset
    
  def epsg(self):
    return self.mEpsg
    
  def setEpsgNum(self, num):
    self.mEpsg = QString("EPSG:"+str(num))
    
  def epsgNum(self):
    return long(QString(self.mEpsg).right(QString(self.mEpsg).indexOf(":")))
    
  def gdalPath(self):
    return self.mGdalDatasetPath
    
  def loadTempFile(self, path = ""):
    if not path == "":
      self.mGdalDatasetPath = path
    """info = QFileInfo( self.mGdalDatasetPath )
    self.mGdalDatasetLayer = QgsRasterLayer( self.mGdalDatasetPath, info.completeBaseName() )"""
    self.mFinishedRendering = True
    
  def setCrsDim(self, crs):
    QgsLogger.debug("*****************************************")
    QgsLogger.debug("*****************************************")
    QgsLogger.debug("TL: "+str(self.TL.x())+" "+str(self.TL.y()))
    QgsLogger.debug("TR: "+str(self.TR.x())+" "+str(self.TR.y()))
    QgsLogger.debug("BR: "+str(self.BR.x())+" "+str(self.BR.y()))
    QgsLogger.debug("BL: "+str(self.BL.x())+" "+str(self.BL.y()))
    
    ct = QgsCoordinateTransform()
    cs = QgsCoordinateReferenceSystem()
    cs.createFromEpsg(self.epsgNum())
    ct.setSourceCrs(cs)
    ct.setDestCRS(crs)
    
    point1 = ct.transform(self.TL)
    point2 = ct.transform(self.TR)
    point3 = ct.transform(self.BR)
    point4 = ct.transform(self.BL)
    """res = self.calculateWorldData(self.TL, self.TR, self.BR, self.BL)
    
    p1 = ct.transform(QgsPoint(res[2], res[1]))
    p2 = ct.transform(QgsPoint(res[0], res[3]))
    
    rotationY = p1.x()
    rotationX = p1.y()
    pixelX = p2.x()
    pixelY = p2.y()"""
    QgsLogger.debug("*****************************************")
    QgsLogger.debug("*****************************************")
    #QgsLogger.debug(str(rotationY)+" "+str(pixelX)+" "+str(pixelY))
    #point1.setY(point1.y()+1000)
    
    res = self.calculateWorldData(point1, point2, point3, point4)
    rotationY = res[2]
    rotationX = res[1]
    pixelX = res[0]
    pixelY = res[3]
    
    #This takes an average of converting the coordinate pre and post. More accurate
    
    """res2 = self.calculateWorldData(point1, point2, point3, point4)
    rotationY = (res2[2]+p1.x())/2
    rotationX = (res2[1]+p1.y())/2
    pixelX = (res2[0]+p2.x())/2
    pixelY = (res2[3]+p2.y())/2"""

    QgsLogger.debug("*****************************************")
    QgsLogger.debug("trans: "+str(point1.x())+" "+str(point1.y()))
    """a = abs(sqrt( (point1.x() - point2.x()) * (point1.x() - point2.x()) + (point1.y() - point2.y()) * (point1.y() - point2.y()) ) / self.mColumns)
    e = 0 - abs(sqrt( (point1.x() - point4.x()) * (point1.x() - point4.x()) + (point1.y() - point4.y()) * (point1.y() - point4.y()) ) / self.mRows) 
    c = point1.x()
    f = point1.y()
    b = (a * self.mColumns + c - point3.x() ) / self.mRows * -1
    d = (e * self.mRows + f - point3.y() ) / self.mColumns * -1"""
	    
    path = self.mImagePath.left(self.mImagePath.length()-4)+".jgw"
    
    #WorldFiler(path, self.pixelX, self.rotationY, self.rotationX, self.pixelY, self.TL.x(), self.TL.y())
    #WorldFiler(path, a, d, -1*b, -1*e, c, f)
    
    """rY = rotationY
    if rY > 0:
      rY *= -1
    rX = rotationX
    if rX > 0:
      rX *= -1
    pixelX += rX
    pixelY += rY
    """
    WorldFiler(self.mImagePath.left(self.mImagePath.length()-4)+".jgw", pixelX, rotationX, rotationY, pixelY, point1.x(), point1.y())
    
  def setCrsWorld(self, TL, pathWorld, crs):
    ct = QgsCoordinateTransform()
    cs = QgsCoordinateReferenceSystem()
    cs.createFromEpsg(self.epsgNum())
    ct.setSourceCrs(cs)
    ct.setDestCRS(crs)
    QgsLogger.debug("++++++++++++++++++++++++++++++++++++++0")
    self.readWorldFileData(pathWorld)
    QgsLogger.debug("++++++++++++++++++++++++++++++++++++++1")
    point1 = ct.transform(self.TL)
    QgsLogger.debug("++++++++++++++++++++++++++++++++++++++2 ")
    point2 = ct.transform(QgsPoint(self.pixelX, self.pixelY))
    QgsLogger.debug("++++++++++++++++++++++++++++++++++++++3")
    point3 = ct.transform(QgsPoint(self.rotationX, self.rotationY))
    self.mSourcePath = self.mImagePath.left(self.mImagePath.length()-4)+".jgw"

    WorldFiler(self.mSourcePath, point2.x(), point3.y(), point3.x(), point2.y(), point1.x(), point1.y())
    #WorldFiler(self.mSourcePath, self.pixelX, self.rotationY, self.rotationX, self.pixelY, self.TL.x(), self.TL.y())

  def xAddition(self):
    res = self.rotationX
    if res < 0:
      return 0-res
    return res
    
  def yAddition(self):
    res = self.rotationY
    if res < 0:
      return 0-res
    return res
 
  def readCoordinates(self, path):
    reader = CoordinateReader()
    success = reader.readCoordinates(path)
    self.mRows = reader.rows()
    self.mColumns = reader.columns()
    self.TL = QgsPoint(reader.topLeftX(), reader.topLeftY())
    self.TR = QgsPoint(reader.topRightX(), reader.topRightY())
    self.BR = QgsPoint(reader.bottomRightX(), reader.bottomRightY())
    self.BL = QgsPoint(reader.bottomLeftX(), reader.bottomLeftY())
    self.mEpsg = reader.epsg()
    return success
  
  def referenceExtent(self):
    ct = QgsCoordinateTransform()
    cs = QgsCoordinateReferenceSystem()
    cs.createFromEpsg(self.epsgNum())
    ct.setSourceCrs(cs)
    ct.setDestCRS(self.refcrs)
    
    point1 = ct.transform(self.TL)
    newPoint = QgsPoint(self.TL.x()+(self.pixelX*self.mColumns), self.TL.y()+(self.pixelY*self.mRows))
    point2 = ct.transform(newPoint)
    return QgsRectangle(point1, point2)
  
  def createOriginalAttributes(self):
    if self.hasSourceFile():
      try:

	#Take the average of the top/bottom and left/right coordinates
	if self.hasDimSource:
	  res = self.calculateWorldData(self.TL, self.TR, self.BR, self.BL)
	  self.rotationY = res[1]
	  self.rotationX = res[2]
	  self.pixelX = res[0]
	  self.pixelY = res[3]
	  ct = QgsCoordinateTransform()
	  cs = QgsCoordinateReferenceSystem()
	  cs.createFromEpsg(self.epsgNum())
	  ct.setSourceCrs(cs)
	  ct.setDestCRS(self.refcrs)
	  
	  point1 = ct.transform(self.TL)
	  point2 = ct.transform(self.TR)
	  point3 = ct.transform(self.BR)
	  point4 = ct.transform(self.BL)

	  a = abs(sqrt( (point1.x() - point2.x()) * (point1.x() - point2.x()) + (point1.y() - point2.y()) * (point1.y() - point2.y()) ) / self.mColumns)
	  e = 0 - abs(sqrt( (point1.x() - point4.x()) * (point1.x() - point4.x()) + (point1.y() - point4.y()) * (point1.y() - point4.y()) ) / self.mRows) 
	  c = point1.x()
	  f = point1.y()
	  b = (a * self.mColumns + c - point3.x() ) / self.mRows * -1
	  d = (e * self.mRows + f - point3.y() ) / self.mColumns * -1
	    
	  self.mSourcePath = self.mImagePath.left(self.mImagePath.length()-4)+".jgw"
	    
	  """xResolution = 0.000005
	  yResolution = 0.000005#0.000005697
	    
	  angleRad = -math.radians(1.47689536467558)
	  line1 = xResolution * math.cos(angleRad)
	  line2 = -xResolution * math.sin(angleRad)
	  line3 = -yResolution * math.sin(angleRad)
	  line4 = -yResolution * math.cos(angleRad)
	  line5 = self.TL.x()
	  line6 = self.TL.y()
	  WorldFiler(self.mSourcePath, line1, line2, line3, line4, line5, line6)"""

	  #creator = WorldFileCreator(self.tifPath, self.mSourcePath, self.epsgNum(), self.refcrs.epsg())
	  #creator.createWorldFile()
	  #WorldFiler(self.mSourcePath, a, d, -1*b, -1*e, c, f)
	  WorldFiler(self.mSourcePath, self.pixelX, self.rotationY, self.rotationX, self.pixelY, self.TL.x(), self.TL.y())
	"""elif self.hasWorldSource:
	  if self.mEpsg == "EPSG:4326":
	    if self.TL.x() > 180 or self.TL.x() < -180 or self.TL.y() > 90 or self.TL.y() < -90:
	      self.invalidCoordinates = True
	      return False
	  ct = QgsCoordinateTransform()
	  cs = QgsCoordinateReferenceSystem()
	  cs.createFromEpsg(self.epsgNum())
	  ct.setSourceCrs(cs)
	  ct.setDestCRS(self.refcrs)
	  point1 = ct.transform(self.TL)
	  point2 = ct.transform(QgsPoint(self.pixelX, self.pixelY))
	  point3 = ct.transform(QgsPoint(self.rotationX, self.rotationY))
	  self.mSourcePath = self.mImagePath.left(self.mImagePath.length()-4)+".jgw"
	  shutil.copy2(self.mSourcePath, self.mSourcePath+".tmp")
	  #WorldFiler(self.mSourcePath, point2.x(), point3.y(), point3.x(), point2.y(), point1.x(), point1.y())
	"""
	QgsLogger.debug( "Creating world file", 1, "thumbnail.py", "SumbandilaSat Georeferencer-Thumbnail")
	self.invalidCoordinates = False
	return True
      except BaseException as ee:
	QgsLogger.debug("************error: "+str(ee))
	self.invalidCoordinates = True
	return False
    
  def restoreDefaultWorldFile(self):
    if self.hasWorldSource:
      if os.path.isfile(self.mSourcePath+".tmp"):
	#shutil.copy2(self.mSourcePath+".tmp", self.mSourcePath)
	#os.remove(self.mSourcePath+".tmp")
	pass
    
  def calculateWorldData(self, TL, TR, BR, BL):
    pixelX = (TL.x() - TR.x()) / (0 - self.mColumns)
    rotationY = (TL.y() - TR.y()) / (0 - self.mColumns)
    rotationX = (TL.x() - BL.x()) / (0 - self.mRows)
    pixelY = (TL.y() - BL.y()) / (0 - self.mRows)
    if self.flip:
      pixelY *= -1
      rotationX *= -1
    return (pixelX, rotationY, rotationX, pixelY)
    
  def updateAttributes(self, referencePoint, thumbnailPoint):
    #Cacluate the diffrence between the reference and thumbnail point
    
    rX = referencePoint.x()
    if rX < 0:
      rX *= -1
    rY = referencePoint.y()
    if rY < 0:
      rY *= -1
    tX = thumbnailPoint.x()
    if tX < 0:
      tX *= -1
    tY = thumbnailPoint.y()
    if tY < 0:
      tY *= -1
      
    self.mXOffset = 0.0
    self.mYOffset = 0.0
      
    if referencePoint.x() < thumbnailPoint.x():
      if rX > tX:
	self.mXOffset = rX - tX
      elif rX < tX:
	self.mXOffset = tX - rX
    elif referencePoint.x() > thumbnailPoint.x():
      if rX > tX:
	self.mXOffset = tX - rX
      elif rX < tX:
	self.mXOffset = rX - tX
	  
    if referencePoint.y() > thumbnailPoint.y():
      if rY > tY:
	self.mYOffset = tY - rY
      elif rY < tY:
	self.mYOffset = rY - tY
    elif referencePoint.y() < thumbnailPoint.y():
      if rY > tY:
	self.mYOffset = rY - tY
      elif rY < tY:
	self.mYOffset = tY - rY
	
    """rotationY = (self.TL.y() - self.TR.y()) / (0 - self.mColumns)
    rotationY *= self.mColumns
    QgsLogger.debug("*****************************************++++++++++++++++++++++++++++++++++ "+str(rotationY))
    QgsLogger.debug("yOffset: "+str(self.mYOffset))
    self.mYOffset += rotationY
    QgsLogger.debug("yOffset: "+str(self.mYOffset))"""
	
    self.topLeftXNew = -self.mXOffset + self.TL.x()
    self.topLeftYNew = -self.mYOffset + self.TL.y()
    self.topRightXNew = -self.mXOffset + self.TR.x()
    self.topRightYNew = -self.mYOffset + self.TR.y()
    self.bottomRightXNew = -self.mXOffset + self.BR.x()
    self.bottomRightYNew = -self.mYOffset + self.BR.y()
    self.bottomLeftXNew = -self.mXOffset + self.BL.x()
    self.bottomLeftYNew = -self.mYOffset + self.BL.y()
    #Take the average of the top/bottom and left/right coordinates
    self.newTL = QgsPoint(self.topLeftXNew, self.topLeftYNew)
    self.newTR = QgsPoint(self.topRightXNew, self.topRightYNew)
    self.newBR = QgsPoint(self.bottomRightXNew, self.bottomRightYNew)
    self.newBL = QgsPoint(self.bottomLeftXNew, self.bottomLeftYNew)
      
    ct = QgsCoordinateTransform()
    cs = QgsCoordinateReferenceSystem()
    cs.createFromEpsg(self.epsgNum())
    ct.setSourceCrs(cs)
    
    cs2 = QgsCoordinateReferenceSystem()
    cs2.createFromEpsg(900913)
    ct.setDestCRS(cs2)
    
    if self.hasDimSource:
      res = self.calculateWorldData(self.newTL, self.newTR, self.newBR, self.newBL)
      self.pixelX = res[0]
      self.rotationY = res[1]
      self.rotationX = res[2]
      self.pixelY = res[3]

      #WorldFiler(self.mSourcePath, b, d, a, e, c, f)
      #WorldFiler(self.mSourcePath, a, d, -1*b, -1*e, c, f)
      WorldFiler(self.mSourcePath, self.pixelX, self.rotationY, self.rotationX, self.pixelY, self.newTL.x(), self.newTL.y())
      #self.writeWorldFile( self.mImagePath.left(self.mImagePath.length()-4)+".jgw", self.pixelX, self.rotationY, self.rotationX, self.pixelY, self.topLeftXNew, self.topLeftYNew)
    elif self.hasWorldSource:
      self.updateWorldFile( self.mImagePath.left(self.mImagePath.length()-4)+".jgw", self.topLeftXNew, self.topLeftYNew)
    QgsLogger.debug( "Updating world file. Offset: (" + QString.number( self.mXOffset, 'f', 10 ) + ", " + QString.number( self.mYOffset, 'f', 10 ) + ")", 1, "thumbnail.py", "SumbandilaSat Georeferencer-Thumbnail")
    self.wasGeoreferenced = True
      
  def writeWorldFile(self, path, pixelX, rotationY, rotationX, pixelY, mTopLeftX, mTopLeftY):
    WorldFiler(path, pixelX, rotationY, rotationX, pixelY, mTopLeftX, mTopLeftY)
    
  def updateWorldFile(self, path, mTopLeftX, mTopLeftY):
    WorldFiler(path, self.pixelX, self.rotationY, self.rotationX, self.pixelY, self.topLeftXNew, self.topLeftYNew)
    
  def readJpegDimension(self, path = ""):
    if path == "":
      path = self.mImagePath
    img = QImage(path)
    self.mRows = img.height()
    self.mColumns = img.width()
    
  def readWorldFileData(self, path = ""):
    if path == "":
      path = self.mSourcePath
    f = open(path, "r")
    counter = 1
    for line in f:
      if counter == 1:
	self.pixelX = float(str(line).strip())
      if counter == 2:
	self.rotationY = float(str(line).strip())
      if counter == 3:
	self.rotationX = float(str(line).strip())
      if counter == 4:
	self.pixelY = float(str(line).strip())
      if counter == 5:
	self.TL.setX(float(str(line).strip()))
      elif counter == 6:
	self.TL.setY(float(str(line).strip()))
      counter += 1
    f.close()
