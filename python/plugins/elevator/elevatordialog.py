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
from qgis.core import *
from qgis.gui import *
from ui_elevatordialog import Ui_ElevatorDialog
from coordinateanalyzer import *
import urllib2
import threading
import StringIO
import httplib
import os.path
import os
import zipfile

class AccessThread(threading.Thread):
  
  def __init__(self, parent, threadId = -1, urls = ""):
    threading.Thread.__init__(self)
    self.urls = urls
    self.threadId = threadId
    self.condition = "NotStarted"

  def removeFirst(self):
    if len(self.urls) > 0:
      self.urls.pop(0)
    return len(self.urls)

  def run(self):
    self.condition = "Started"
    try:
      urllib2.urlopen(str(self.urls[0]))
      self.condition = "Succeeded"
    except urllib2.URLError:
      self.condition = "Failed"
      if len(self.urls) <= 1:
	self.condition = "FinalFailed"
    except BaseException as ex:
      pass

class ElevatorDialog(QDialog):
  
  def __init__(self, iface, version):
    self.iface = iface
    QDialog.__init__( self, self.iface.mainWindow() )
    self.ui = Ui_ElevatorDialog()
    self.ui.setupUi( self )
    
    self.server = "http://dds.cr.usgs.gov/srtm/version2_1/SRTM3/"
    
    QObject.connect(self.ui.locationComboBox, SIGNAL("currentIndexChanged(int)"), self.updateUrl)
    QObject.connect(self.ui.tlX, SIGNAL("textChanged(QString)"), self.updateUrlText)
    QObject.connect(self.ui.tlY, SIGNAL("textChanged(QString)"), self.updateUrlText)
    QObject.connect(self.ui.trX, SIGNAL("textChanged(QString)"), self.updateUrlText)
    QObject.connect(self.ui.trY, SIGNAL("textChanged(QString)"), self.updateUrlText)
    QObject.connect(self.ui.blX, SIGNAL("textChanged(QString)"), self.updateUrlText)
    QObject.connect(self.ui.blY, SIGNAL("textChanged(QString)"), self.updateUrlText)
    QObject.connect(self.ui.brX, SIGNAL("textChanged(QString)"), self.updateUrlText)
    QObject.connect(self.ui.brY, SIGNAL("textChanged(QString)"), self.updateUrlText)
    QObject.connect(self.ui.crsButton, SIGNAL("clicked()"), self.selectCrs)
    QObject.connect(self.ui.extentButton, SIGNAL("clicked()"), self.setExtent)
    QObject.connect(self.ui.testButton, SIGNAL("clicked()"), self.updateConnection)
    QObject.connect(self.ui.outputButton, SIGNAL("clicked()"), self.selectFile)
    
    QObject.connect(self.ui.backButton, SIGNAL("clicked()"), self.goBack)
    QObject.connect(self.ui.nextButton, SIGNAL("clicked()"), self.goNext)
    QObject.connect(self.ui.startButton, SIGNAL("clicked()"), self.start)
    QObject.connect(self.ui.stopButton, SIGNAL("clicked()"), self.stop)
    QObject.connect(self.ui.helpButton, SIGNAL("clicked()"), self.help)
    QObject.connect(self.ui.tabWidget, SIGNAL("currentChanged(int)"), self.changeTab)
    
    self.ui.unavailableWidget.hide()
    self.ui.partlyWidget.hide()
    self.ui.availableWidget.hide()
    self.ui.busyWidget.hide()
    self.ui.tabWidget.setCurrentIndex(0)
    
    self.tiles = []
    self.threads = []
    
    self.crs = QgsCoordinateReferenceSystem(4326)
    self.ui.crsLabel.setText(str(self.crs.description())+" ("+str(self.crs.srsid())+")")
    self.getDemInfo()
    self.updateUrl()
    self.deleteTempFiles()
    
  def showMessage(self, string, type = "info"):
    msgbox = QMessageBox(self.iface.mainWindow())
    msgbox.setText(string)
    msgbox.setWindowTitle("Sumbandila")
    if type == "info":
      msgbox.setIcon(QMessageBox.Information)
    elif type == "error":
      msgbox.setIcon(QMessageBox.Critical)
    elif type == "warning":
      msgbox.setIcon(QMessageBox.Warning)
    elif type == "question":
      msgbox.setIcon(QMessageBox.Question)
    elif type == "none":
      msgbox.setIcon(QMessageBox.NoIcon)
    msgbox.setModal(True)
    msgbox.exec_()
    
  def changeTab(self, i = 0):
    index = self.ui.tabWidget.currentIndex()
    self.ui.startButton.hide()
    self.ui.stopButton.hide()
    self.ui.backButton.hide()
    self.ui.nextButton.hide()
    if index == 0:
      self.ui.nextButton.show()
    elif index == 3:
      self.ui.backButton.show()
      self.ui.startButton.show()
    else:
      self.ui.backButton.show()
      self.ui.nextButton.show()
      
  def goBack(self):
    if self.ui.tabWidget.currentIndex() > 0:
      self.ui.tabWidget.setCurrentIndex(self.ui.tabWidget.currentIndex()-1)
      
  def goNext(self):
    if self.ui.tabWidget.currentIndex() < 3:
      self.ui.tabWidget.setCurrentIndex(self.ui.tabWidget.currentIndex()+1)
    
  def selectFile(self):
    fileName = QFileDialog.getSaveFileName(self, "Save Output Image")
    if fileName != "":
      self.ui.outputLine.setText(fileName)
    
  def setExtent(self):
    ct = QgsCoordinateTransform()
    self.crs = self.iface.mapCanvas().mapRenderer().destinationSrs()
    self.ui.crsLabel.setText(str(self.crs.description())+" ("+str(self.crs.srsid())+")")
    cs2 = QgsCoordinateReferenceSystem(self.crs)
      
    ct.setSourceCrs(self.crs)
    ct.setDestCRS(cs2)
      
    point1 = ct.transform(QgsPoint(self.iface.mapCanvas().extent().xMinimum(), self.iface.mapCanvas().extent().yMinimum()))
    point2 = ct.transform(QgsPoint(self.iface.mapCanvas().extent().xMaximum(), self.iface.mapCanvas().extent().yMaximum()))
    
    self.ui.tlX.setText(str(point1.x()))
    self.ui.tlY.setText(str(point1.y()))
    self.ui.trX.setText(str(point2.x()))
    self.ui.trY.setText(str(point1.y()))
    self.ui.brX.setText(str(point2.x()))
    self.ui.brY.setText(str(point2.y()))
    self.ui.blX.setText(str(point1.x()))
    self.ui.blY.setText(str(point2.y()))

  def selectCrs(self):
    dialog = QgsGenericProjectionSelector(self)
    if dialog.exec_():
      self.referenceEpsg = dialog.selectedEpsg()
      self.crs = QgsCoordinateReferenceSystem(self.referenceEpsg)
      self.ui.crsLabel.setText(str(self.crs.description())+" ("+str(self.referenceEpsg)+")")

  def deleteTempFiles(self):
    if not QDir(QDir.temp().absolutePath()+"/elevator").exists():
      QDir().mkdir(QDir.temp().absolutePath()+"/elevator")
    myDir = QDir( QDir.temp().absolutePath()+"/elevator/" )
    fileList = myDir.entryInfoList("*")
    for theFile in fileList:
      f = QFile(theFile.absoluteFilePath())
      f.remove()

  def getDemInfo(self):
    location = self.ui.locationComboBox.itemText(self.ui.locationComboBox.currentIndex())
    if location == "Africa":
      location = "Africa"
    elif location == "Australia":
      location = "Australia"
    elif location == "Islands":
      location = "Islands"
    elif location == "North America":
      location = "North_America"
    elif location == "South America":
      location = "South_America"
    elif location == "Eurasia":
      location = "Eurasia"
    wgs84 = QgsCoordinateReferenceSystem(4326)
    xform = QgsCoordinateTransform(self.crs, wgs84)
    tl = QgsPoint(self.ui.tlX.text().toDouble()[0],self.ui.tlY.text().toDouble()[0])
    tr = QgsPoint(self.ui.trX.text().toDouble()[0],self.ui.trY.text().toDouble()[0])
    br = QgsPoint(self.ui.brX.text().toDouble()[0],self.ui.brY.text().toDouble()[0])
    bl = QgsPoint(self.ui.blX.text().toDouble()[0],self.ui.blY.text().toDouble()[0])
    
    self.pointTL = xform.transform(tl)
    self.pointTR = xform.transform(tr)
    self.pointBR = xform.transform(br)
    self.pointBL = xform.transform(bl)

    self.baseurl = self.server + location
    self.location = location
    
  def showConnection(self, condition = ""):
    self.ui.unavailableWidget.hide()
    self.ui.partlyWidget.hide()
    self.ui.availableWidget.hide()
    """if self.thread1 != None and self.thread2 != None and self.thread3 != None and self.thread4 != None:
      if self.thread1.condition != "NotStarted" and self.thread2.condition != "NotStarted" and self.thread3.condition != "NotStarted" and self.thread4.condition != "NotStarted":
	self.ui.busyWidget.hide()
	if self.thread1.condition == "Succeeded" and self.thread2.condition == "Succeeded" and self.thread3.condition == "Succeeded" and self.thread4.condition == "Succeeded":
	  self.ui.availableWidget.show()
	elif self.thread1.condition == "Failed" and self.thread2.condition == "Failed" and self.thread3.condition == "Failed" and self.thread4.condition == "Failed":
	  self.ui.unavailableWidget.show()
	else:
	  self.ui.partlyWidget.show()
    self.ui.connectionWidget.adjustSize()
    """
    none = False
    for thread in self.threads:
      if thread == None:
	none = True
	break
    none = True
    QgsLogger.debug("goooo: 1")
    if none:
      QgsLogger.debug("goooo: 2")
      self.ui.busyWidget.hide()
      allSuccess = 0
      allFail = 0
      for thread in self.threads:
	if thread.condition == "Succeeded":
	  allSuccess += 1
	if thread.condition == "FinalFailed":
	  allFail += 1
      if allSuccess == len(self.threads):
	self.ui.availableWidget.show()
      elif allFail == len(self.threads):
	self.ui.unavailableWidget.show()
      else:
	self.ui.partlyWidget.show()
    self.ui.connectionWidget.adjustSize()
    
  def updateConnection(self, url = ""):
    self.calcDiffrence()
    
    if self.ui.locationComboBox.itemText(self.ui.locationComboBox.currentIndex()) == "Automatic detection":
      self.locations = [self.server+"Africa/", self.server+"Australia/", self.server+"Islands/", self.server+"North_America/", self.server+"South_America/", self.server+"Eurasia/"]
    else:
      self.locations = [self.server+self.ui.locationComboBox.itemText(self.ui.locationComboBox.currentIndex()).replace(" ", "_")+"/"]
    
    self.threads = []
    
    for i in range(len(self.tiles)):
      thread = AccessThread(self)
      thread.threadId = i
      urls = []
      for location in self.locations:
	urls.append(location + self.tiles[i] + ".hgt.zip")
      thread.urls = urls
      self.threads.append(thread)
      self.threads[i].start()
     
    for thread in self.threads:
      thread.join()
      
    while self.threadsAreBusy():
      pass
    
    self.urls = []
    for thread in self.threads:
      if len(thread.urls) > 0:
	self.urls.append(thread.urls[0])
    self.showConnection()
     
  def threadsAreBusy(self):
    for thread in self.threads:
      if (not thread.isAlive()) and thread.condition != "FinalFailed" and thread.condition != "Succeeded":
	if thread.removeFirst() > 0:
	  newThread = AccessThread(self)
	  newThread.threadId = thread.threadId
	  newThread.urls = thread.urls
	  self.threads[thread.threadId] = newThread
	  self.threads[thread.threadId].start()
	  self.threads[thread.threadId].join()
	return True
    for thread in self.threads:
      if thread.isAlive():
	return True
    return False
    
  def updateUrlText(self, text):
    self.updateUrl()
    #self.updateConnection()
     
  def updateUrl(self, index = 0):
    self.getDemInfo()
    self.ui.tlUrl.setText("")
    self.ui.trUrl.setText("")
    self.ui.brUrl.setText("")
    self.ui.blUrl.setText("")
    if self.ui.tlX.text() != "" or self.ui.tlY.text() != "":
      self.ui.tlUrl.setText(self.getUrl("TL"))
    if self.ui.trX.text() != "" or self.ui.trY.text() != "":
      self.ui.trUrl.setText(self.getUrl("TR"))
    if self.ui.brX.text() != "" or self.ui.brY.text() != "":
      self.ui.brUrl.setText(self.getUrl("BR"))
    if self.ui.blX.text() != "" or self.ui.blY.text() != "":
      self.ui.blUrl.setText(self.getUrl("BL"))
     
  def getUrl(self, corner):
    return '/'.join((str(self.baseurl), str(self.calcFilename(corner))))
    
  def start(self):
    self.updateConnection()
    self.setCursor(Qt.WaitCursor)
    command = "python "+os.path.dirname( __file__ ).replace("\\", "/")+"/merger.py "
    counter = 0
    for url in self.urls:
      name = QString(url)
      index = name.lastIndexOf("/")
      path = QDir.temp().absolutePath()+"/elevator/"
      name = name.right(name.length()-index-1)
      self.downloadFile(url, str(path+name))
      sourceZip = zipfile.ZipFile(str(path+name), 'r')
      sourceZip.extractall(str(path))
      command += path+name.replace(".zip", "")+" "
      counter += 1
      self.ui.progressBar.setValue((counter/(len(self.urls)*1.0))*100.0)
    process = QProcess(self)
    QObject.connect(process, SIGNAL("finished(int, QProcess::ExitStatus)"), self.mergeCompleted)
    process.start(command+" -o "+self.ui.outputLine.text())
    
  def stop(self):
    pass
  
  def help(self):
    pass
    
  def mergeCompleted(self, i, status):
    self.setCursor(Qt.ArrowCursor)
    
  def downloadFile(self, url, path):
    try:
      u = urllib2.urlopen(str(url))
      buf = u.read()
      sio = StringIO.StringIO(buf)
      out = open(path,'wb')
      out.write(buf)
      out.close()
    except urllib2.URLError as ex:
      self.showMessage("Could not connect to the server\n"+str(ex), "error")
    
  def calcDiffrence(self):
    self.tiles = []
    tl = HeightPoint()
    tl.detectFromPath(self.ui.tlUrl.text())
    tr = HeightPoint()
    tr.detectFromPath(self.ui.trUrl.text())
    br = HeightPoint()
    br.detectFromPath(self.ui.brUrl.text())
    bl = HeightPoint()
    bl.detectFromPath(self.ui.blUrl.text())
    corners = HeightCollection(tl, tr, br, bl)
    ca = CoordinateAnalayzer(corners)
    for s in ca.allNames:
      self.tiles.append(s)
    
  def calcFilename(self, corner):
    point = QPoint(0,0)
    if corner == "TL":
      point = self.pointTL
    elif corner == "TR":
      point = self.pointTR
    elif corner == "BR":
      point = self.pointBR
    elif corner == "BL":
      point = self.pointBL
    latprefix = 'N'
    longprefix = 'E'
    ilong = int(point.x())
    ilat = int(point.y())
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