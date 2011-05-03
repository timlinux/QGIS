#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
rectificationdialogbase.py - The class providing the graphical user interface
for the AutoGCP plugin's rectification dialog. All the necessary information
required to manually orthorectify or geo-transform a raw image with a gcp
set is provided here.
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
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from helpviewer import *
from qgis.core import *
from ui_demdialogbase import Ui_DemDialog
import urllib2
import StringIO

class DemDialog(QDialog):
  
  def __init__(self, parent, canvas):
    QDialog.__init__( self, parent )
    self.ui = Ui_DemDialog()
    self.ui.setupUi( self )
    
    try:
      self.canvas = canvas
      ext = self.canvas.extent()
      self.ui.minX.setText(QString('%s'%ext.xMinimum()))
      self.ui.maxX.setText(QString('%s'%ext.xMaximum()))
      self.ui.maxY.setText(QString('%s'%ext.yMaximum()))
      self.ui.minY.setText(QString('%s'%ext.yMinimum()))
      QObject.connect(self.ui.buttonBox, SIGNAL("rejected()"), self.close)
      QObject.connect(self.ui.buttonBox, SIGNAL("accepted()"), self.getFile)
      QObject.connect(self.ui.testButton, SIGNAL("clicked()"), self.testDem)
      QObject.connect(self.ui.locationComboBox, SIGNAL("currentIndexChanged(int)"), self.updateUrl)
      QObject.connect(self.ui.minX, SIGNAL("textChanged(QString)"), self.updateUrlText)
      QObject.connect(self.ui.minY, SIGNAL("textChanged(QString)"), self.updateUrlText)
      QObject.connect(self.ui.maxX, SIGNAL("textChanged(QString)"), self.updateUrlText)
      QObject.connect(self.ui.maxY, SIGNAL("textChanged(QString)"), self.updateUrlText)
      self.getDemInfo()
      self.updateUrl()
      self.deleteTempFiles()
    except:
      QgsLogger.debug( "Setting up the dialog failed: ", 1, "demdialog.py", "DemDialog::__init__()")

  def deleteTempFiles(self):
    if not QDir(QDir.temp().absolutePath()+"/qgis_autogcp").exists():
      QDir().mkdir(QDir.temp().absolutePath()+"/qgis_autogcp")
    myDir = QDir( QDir.temp().absolutePath()+"/qgis_autogcp/" )
    fileList = myDir.entryInfoList("*")
    for theFile in fileList:
      f = QFile(theFile.absoluteFilePath())
      f.remove()

  def getDemInfo(self):
    try:
      location = self.ui.locationComboBox.itemText(self.ui.locationComboBox.currentIndex())
      if location == "Africa":
	location = "SRTM3/Africa"
      elif location == "Australia":
	location = "SRTM3/Australia"
      elif location == "Islands":
	location = "SRTM3/Islands"
      elif location == "North America":
	location = "SRTM3/North_America"
      elif location == "South America":
	location = "SRTM3/South_America"
      elif location == "Eurasia":
	location = "SRTM3/Eurasia"
      wgs84 = QgsCoordinateReferenceSystem(4326)
      src = self.canvas.mapRenderer().destinationSrs()
      self.xform = QgsCoordinateTransform(src,wgs84)
      self.xforminv = QgsCoordinateTransform(wgs84,src)
      lu = QgsPoint(float(self.ui.minX.text()),float(self.ui.minY.text()))
      ro = QgsPoint(float(self.ui.maxX.text()),float(self.ui.maxY.text()))
	
      self.leftbottom = self.xform.transform(lu)
      self.righttop = self.xform.transform(ro)

      x = self.leftbottom.x()
      y = self.leftbottom.y()
	
      self.refpoint = QgsPoint(x, y)
      self.baseurl = 'http://dds.cr.usgs.gov/srtm/version2_1/' + location
      self.location = location
    except:
      QgsLogger.debug( "Getting DEM info failed: ", 1, "demdialog.py", "DemDialog::getDemInfo()")

  def testDem(self):
    try:
      self.getDemInfo()
      if self.testAvailability():
	self.ui.iconLabel.setPixmap(QPixmap(":/icons/connect.png"))
	self.ui.textLabel.setText("Available")
      else:
	self.ui.iconLabel.setPixmap(QPixmap(":/icons/disconnect.png"))
	self.ui.textLabel.setText("Not available")
    except:
      QgsLogger.debug( "Testing DEM failed: ", 1, "demdialog.py", "DemDialog::testDem()")
    
  def testAvailability(self):
    try:
      u = urllib2.urlopen(str(self.ui.url.text()))
      return True
    except urllib2.URLError,e:
      return False
     
  def updateUrlText(self, text):
    self.updateUrl()
     
  def updateUrl(self, index = 0):
    self.getDemInfo()
    self.ui.url.setText(self.getUrl())
     
  def getUrl(self):
    return '/'.join((str(self.baseurl), str(self.calcFilename())))
    
  def getFile(self):
    if self.testAvailability():
      self.setCursor(Qt.WaitCursor)
      url = self.getUrl()
      u = urllib2.urlopen(url)
      buf = u.read()
      sio = StringIO.StringIO(buf)
      temp = QFile(QDir.temp().absolutePath()+"/qgis_autogcp/"+self.calcFilename())
      out = open(temp.fileName(),'wb')
      out.write(buf)
      out.close()
      self.emit(SIGNAL("demDownloaded(QString)"), QString(temp.fileName()))
    
  def calcFilename(self):
    try:
      latprefix = 'N'
      longprefix = 'E'
      ilong = int(self.refpoint.x())
      ilat = int(self.refpoint.y())
      startx = float(ilong)
      starty = float(ilat)
      if ilong < 0:
	ilong =- ilong
	longprefix = 'W'
      if ilong > 180:
	ilong = 360 - ilong
	longprefix = 'W'
      if ilat < 0:
	ilat =- ilat
	latprefix = 'S'
      res = '%s%02d%s%03d.hgt.zip' %(latprefix,ilat,longprefix,ilong)
      return res
    except:
      QgsLogger.debug( "Calculating filename failed: ", 1, "demdialog.py", "DemDialog::calcFilename()")