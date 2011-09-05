#!python
# -*- coding: utf-8 -*-
"""
***************************************************************************************
georeferencer.py - The main interface for the SumbandilaSat plug-in's georeferencer.
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

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from ui_georeferencer import Ui_Georeferencer
from thumbnail import Thumbnail
from settings import SettingsDialog
from thumbnaillist import ThumbnailListDialog
import osgeo.gdal as gdal
import numpy as np
import sys
import shutil
from worldfiler import WorldFiler


openlayersFound = False
import os.path
if os.path.isfile(QgsApplication.qgisSettingsDirPath()+"python/plugins/openlayers/openlayers_plugin.py"):
  sys.path.insert(0, str(QgsApplication.qgisSettingsDirPath()+"python/plugins/openlayers"))
  OL = __import__('openlayers_plugin', globals(), locals(), []) 
  openlayersFound = True

class GeoreferencerWindow(QMainWindow):

  def __init__(self, iface):
    QMainWindow.__init__( self )
    self.ui = Ui_Georeferencer()
    self.iface = iface
    self.ui.setupUi( self )
    self.ui.dockWidget.setParent(self)
    self.addDockWidget(Qt.RightDockWidgetArea, self.ui.dockWidget)
    self.ui.dockWidget.setMinimumSize(self.width()/2, self.height()/2)
    #self.setWindowFlags( Qt.WindowTitleHint | Qt.CustomizeWindowHint );
    
    if openlayersFound:
      self.ui.actionWidgetGoogle.show()
      self.ui.toolbarGoogle = QToolBar(self)
      self.ui.toolbarGoogle.addAction(self.ui.actionAddGoogle)
      self.ui.actionWidgetGoogle.layout().addWidget( self.ui.toolbarGoogle, 0, Qt.AlignHCenter )
      QObject.connect(self.ui.actionAddGoogle, SIGNAL("triggered()"), self.addGoogleLayer)
    else:
      self.ui.actionWidgetGoogle.hide()
    
    self.ui.toolbar1 = QToolBar(self)
    self.ui.toolbar1.addAction(self.ui.actionOpenDir)
    self.ui.toolbar1.addAction(self.ui.actionAddThumbnail)
    self.ui.toolbar1.addAction(self.ui.actionRemoveThumbnail)
    self.ui.actionWidget1.layout().addWidget( self.ui.toolbar1, 0, Qt.AlignHCenter )
    
    self.ui.toolbar2 = QToolBar(self)
    self.ui.toolbar2.addAction(self.ui.actionAlready)
    self.ui.toolbar2.addAction(self.ui.actionSettings)
    self.ui.actionWidget2.layout().addWidget( self.ui.toolbar2, 0, Qt.AlignHCenter )
    
    self.ui.toolbarCrs = QToolBar(self)
    self.ui.toolbarCrs.addAction(self.ui.actionCrs)
    self.ui.actionWidgetCrs.layout().addWidget( self.ui.toolbarCrs, 0, Qt.AlignLeft )
    
    self.ui.toolbarSelect = QToolBar(self)
    self.ui.toolbarSelect.addAction(self.ui.actionPreviousLayer)
    self.ui.toolbarSelect.addAction(self.ui.actionSelectTable)
    self.ui.toolbarSelect.addAction(self.ui.actionNextLayer)
    self.ui.toolbarSelect.addAction(self.ui.actionSelectPoint)
    self.ui.actionWidgetSelect.layout().addWidget( self.ui.toolbarSelect, 0, Qt.AlignLeft )
    
    self.ui.toolbarAccept = QToolBar(self)
    self.ui.toolbarAccept.addAction(self.ui.actionAcceptPoint)
    self.ui.actionWidgetAccept.layout().addWidget( self.ui.toolbarAccept, 0, Qt.AlignLeft )
    
    self.ui.toolbarMap = QToolBar(self)
    self.ui.toolbarMap.layout().setContentsMargins(0,0,0,0)
    self.ui.toolbarMap.addAction(self.ui.actionZoomIn)
    self.ui.toolbarMap.addAction(self.ui.actionZoomOut)
    self.ui.toolbarMap.addAction(self.ui.actionZoomExtent)
    self.ui.toolbarMap.addAction(self.ui.actionPan)
    self.ui.actionWidgetMap.layout().addWidget( self.ui.toolbarMap, 0, Qt.AlignLeft )
    
    self.ui.thumbnailTableWidget.horizontalHeader().setResizeMode( 0, QHeaderView.Stretch )
    self.ui.thumbnailTableWidget.horizontalHeader().setResizeMode( 1, QHeaderView.Stretch )
    self.ui.thumbnailTableWidget.horizontalHeader().setResizeMode( 2, QHeaderView.Stretch )
    self.ui.thumbnailTableWidget.horizontalHeader().setResizeMode( 3, QHeaderView.Stretch )
    self.ui.thumbnailTableWidget.horizontalHeader().setResizeMode( 4, QHeaderView.Stretch )
    #self.adjustSize()
    
    self.referenceLayerIndex = -1
    
    self.thumbnails = []
    self.previousThumbnailId = ""
    self.currentThumbnailIndex = -1
    self.layerIds = []
    self.selecedReferenceLayerName = ""
    self.selecedReferenceLayer = None
    self.selecedReferenceLayerId = ""
    self.referencePosition = QgsPoint()
    self.resultIsFocused = False
    self.referenceEpsg = 900913
    self.referenceCrs = QgsCoordinateReferenceSystem()
    self.referenceCrs.createFromEpsg(self.referenceEpsg)
    
    self.currentGdalThumbnail = -1
    self.currentGdalThumbnail2 = -1
    self.gdalPath = ""
    self.gdalPath2 = ""
    
    myDir = QDir( QDir.temp().absolutePath()+"/qgis_sumbandilasat/" )
    fileList = myDir.entryInfoList("*")
    for theFile in fileList:
      f = QFile(theFile.absoluteFilePath())
      f.remove()
    
    self.loadResult = True
    self.resultLayerId = ""
    self.resultLayerPath = ""
    self.isRendering = False
    self.isRendering2 = False
    self.isInitialized = False
    
    self.emitPoint = QgsMapToolEmitPoint(self.iface.mapCanvas())
    QObject.connect(self.emitPoint, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.selectReferencePoint)
    QObject.connect(self.iface.mapCanvas(), SIGNAL("mapToolSet(QgsMapTool*)"), self.setMapTool)
    
    self.emitRawPoint = QgsMapToolEmitPoint(self.ui.canvas)
    QObject.connect(self.emitRawPoint, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.selectRawPoint)
    QObject.connect(self.ui.canvas, SIGNAL("mapToolSet(QgsMapTool*)"), self.setMapTool)
    self.referencePoint = QgsPoint()
    
    self.isAdding = False #variable determines if you plug-in added the new reference layer, otherwise an infinte loop will exist
    self.isGeoreferencing = False
    self.loadReferenceLayer()
    
    self.rawPoint = None
    self.refPoint = None
    self.rawMarker = None
    self.refMarker = None
    
    self.connect(self.ui.actionSelectPoint, SIGNAL("triggered()"), self.setGeoreferencing)
    self.connect(self.ui.actionAcceptPoint, SIGNAL("triggered()"), self.acceptPoint)
    
    self.connect(self.iface.mapCanvas(), SIGNAL("layersChanged()"), self.loadReferenceLayer)
    
    self.connect(self.ui.referenceComboBox, SIGNAL("currentIndexChanged(QString)"), self.selectReferenceLayer)
    self.connect(self.ui.actionAddThumbnail, SIGNAL("triggered()"), self.openThumbnails)
    self.connect(self.ui.actionRemoveThumbnail, SIGNAL("triggered()"), self.removeThumbnails)
    self.connect(self.ui.actionPreviousLayer, SIGNAL("triggered()"), self.goToPrevious)
    self.connect(self.ui.actionSelectTable, SIGNAL("triggered()"), self.selectTable)
    self.connect(self.ui.actionNextLayer, SIGNAL("triggered()"), self.goToNext)
    self.connect(self.ui.actionSettings, SIGNAL("triggered()"), self.openSettings)
    self.connect(self.ui.actionAlready, SIGNAL("triggered()"), self.openThumbnailList)
    self.connect(self.ui.actionCrs, SIGNAL("triggered()"), self.selectCrs)
    self.connect(self.ui.actionOpenDir, SIGNAL("triggered()"), self.openDirectory)
    #self.connect(self.ui.buttonBox.button(QDialogButtonBox.Close), SIGNAL("clicked()"), self.close)
    
    self.connect(self.ui.actionZoomIn, SIGNAL("triggered()"), self.zoomIn)
    self.connect(self.ui.actionZoomOut, SIGNAL("triggered()"), self.zoomOut)
    self.connect(self.ui.actionZoomExtent, SIGNAL("triggered()"), self.zoomExtent)
    self.connect(self.ui.actionPan, SIGNAL("triggered()"), self.pan)
    self.toolPan = QgsMapToolPan(self.ui.canvas)
    self.toolPan.setAction(self.ui.actionPan)
    self.toolZoomIn = QgsMapToolZoom(self.ui.canvas, False) # false = in
    self.toolZoomIn.setAction(self.ui.actionZoomIn)
    self.toolZoomOut = QgsMapToolZoom(self.ui.canvas, True) # true = out
    self.toolZoomOut.setAction(self.ui.actionZoomOut)
    
    self.settings = QSettings("QuantumGIS", "SumbandilaSat Georeferencer")
    self.loadSettings()
    
  def enterEvent(self, event):
    self.ui.dockWidget.setMinimumSize(30,30)
    
  def addGoogleLayer(self):
    openLayers = OL.OpenlayersPlugin(self.iface)
    openLayers.initGui()
    openLayers.addGoogleSatellite()
    
  #Displays a message to the user
  def showMessage(self, string, type = "info"):
    msgbox = QMessageBox(self.ui.centralwidget)
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
    
  def loadSettings(self):
    dialog = SettingsDialog(self, self.settings)
    dialog.applySettings()
    self.defaultDir = QString()
    if int(self.settings.value("DirectoryChoice", QVariant(0)).toString()) == 0:
      self.defaultDir = self.settings.value("PreviousDirectory", QVariant("")).toString()
    elif int(self.settings.value("DirectoryChoice", QVariant(0)).toString()) == 1:
      self.defaultDir = self.settings.value("DirectoryPath", QVariant("")).toString()
    self.loadResult = self.settings.value("LoadResult", QVariant(True)).toBool()
    self.createBackups = self.settings.value("CreateBackup", QVariant(True)).toBool()
    self.sourcePath = self.settings.value("StructurePath", QVariant("")).toString()
    if self.settings.value("AcceptPoints", QVariant(True)).toBool():
      self.ui.actionWidgetAccept.hide()
    else:
      self.ui.actionWidgetAccept.show()
    
  def openSettings(self):
    dialog = SettingsDialog(self, self.settings)
    dialog.exec_()
    self.loadSettings()
    
  def zoomIn(self):
    self.ui.canvas.setMapTool(self.toolZoomIn)
    self.ui.actionZoomOut.setChecked(False)
    self.ui.actionPan.setChecked(False)
    
  def zoomOut(self):
    self.ui.canvas.setMapTool(self.toolZoomOut)
    self.ui.actionZoomIn.setChecked(False)
    self.ui.actionPan.setChecked(False)
    
  def zoomExtent(self):
    self.ui.canvas.zoomToFullExtent()
    
  def pan(self):
    self.ui.canvas.setMapTool(self.toolPan)
    self.ui.actionZoomOut.setChecked(False)
    self.ui.actionZoomIn.setChecked(False)
    
  def openThumbnailList(self):
    dialog = ThumbnailListDialog(self, self.settings)
    dialog.exec_()
    
  def setMapTool(self, tool):
    if not isinstance(tool, QgsMapToolEmitPoint):
      #self.ui.actionSelectPoints.setChecked(False)
      pass
    
  def selectCrs(self):
    dialog = QgsGenericProjectionSelector(self)
    if not self.selecedReferenceLayer == None:
      dialog.setSelectedEpsg( self.selecedReferenceLayer.crs().epsg() )
    if dialog.exec_():
      self.referenceEpsg = dialog.selectedEpsg()
      self.referenceCrs.createFromEpsg(self.referenceEpsg)
      self.ui.crsLabel.setText(str(self.referenceCrs.description())+" ("+str(self.referenceEpsg)+")")
    
  def createGdalDataset(self, processNumber, inPath, outPath, sCrs, dCrs, width, height):
    process = QProcess(self)
    if processNumber == 0:
      self.connect(process, SIGNAL("finished(int, QProcess::ExitStatus)"), self.processFinished)
      self.isRendering = True
    elif processNumber == 1:
      self.connect(process, SIGNAL("finished(int, QProcess::ExitStatus)"), self.process2Finished)
      self.isRendering2 = True
    process.start('gdal_translate -of GTiff -outsize '+str(width)+' '+str(height)+' '+inPath+' '+outPath)
    while process.state() == QProcess.Running:
      pass
    QgsLogger.debug("Creating temporary file")
    
  def createGdalThumbnails(self, index, processNumber):
    self.updateStatus( index, "Started" )
    thumbnail = self.thumbnails[index]
    info = QFileInfo( thumbnail.imagePath() )
    #self.resampleThumbnail(thumbnail, QDir.temp().absolutePath()+"/qgis_sumbandilasat/"+info.fileName()+".jpg")
    #self.createWorldFile(thumbnail, QDir.temp().absolutePath()+"/qgis_sumbandilasat/"+info.fileName()+".jgw")
    if not QDir(QDir.temp().absolutePath()+"/qgis_sumbandilasat").exists():
      QDir().mkdir(QDir.temp().absolutePath()+"/qgis_sumbandilasat")
    path = ""
    if processNumber == 0:
      self.gdalPath = QDir.temp().absolutePath()+"/qgis_sumbandilasat/"+info.fileName()+".tif"
      path = self.gdalPath
    elif processNumber == 1:
      self.gdalPath2 = QDir.temp().absolutePath()+"/qgis_sumbandilasat/"+info.fileName()+".tif"
      path = self.gdalPath2
    cols = thumbnail.columns() 
    rows = thumbnail.rows()
    while cols > 2000 or rows > 2000:
      cols /= 2
      rows /= 2
    self.createGdalDataset( processNumber, thumbnail.imagePath(), path,  "EPSG:"+str(self.referenceCrs.epsg()), "EPSG:"+str(self.referenceCrs.epsg()), cols, rows )
    
  def processFinished(self, exitCode, status):
    self.isRendering = False
    if QFile(self.gdalPath).exists:
      """self.resampleThumbnail(self.gdalPath, self.gdalPath+".jpg", False)
      
      tx = self.thumbnails[self.currentGdalThumbnail].xAddition()
      if self.thumbnails[self.currentGdalThumbnail].pixelX < 0:
	tx *= -1
      ty = self.thumbnails[self.currentGdalThumbnail].yAddition()
      if self.thumbnails[self.currentGdalThumbnail].pixelY < 0:
	ty *= -1
      px = (self.thumbnails[self.currentGdalThumbnail].pixelX+ty)*2
      py = (self.thumbnails[self.currentGdalThumbnail].pixelY+tx)*2
      if px < 0:
	px *= -1
      if py < 0:
	py *= -1
      WorldFiler(self.gdalPath+".jgw", px, 0.0, 0.0, py, self.thumbnails[self.currentGdalThumbnail].TL.x()+(tx*self.thumbnails[self.currentGdalThumbnail].mColumns), self.thumbnails[self.currentGdalThumbnail].TL.y()+(ty*self.thumbnails[self.currentGdalThumbnail].mRows))
      """
      #self.thumbnails[self.currentGdalThumbnail].loadTempFile(self.gdalPath)
      self.thumbnails[self.currentGdalThumbnail].loadTempFile(self.gdalPath)
      self.thumbnails[self.currentGdalThumbnail].restoreDefaultWorldFile() #Restore original coordinates
      self.updateStatus( self.currentGdalThumbnail, "Finished" )
      QgsLogger.debug("Temporary file created")
    else:
      self.updateStatus( self.currentGdalThumbnail, "Failed" )
      QgsLogger.debug("Failed to create temporary file")
    for i in range(len(self.thumbnails)):
      if not self.thumbnails[i].isFinishedRendering() and not(self.currentGdalThumbnail2 == i):
	self.currentGdalThumbnail = i
	self.createGdalThumbnails(self.currentGdalThumbnail, 0)
	break
      
  #This slot will handle every second thumbnail, which allows to thumbnails to be rendered in parallel
  def process2Finished(self, exitCode, status):
    self.isRendering2 = False
    if QFile(self.gdalPath2).exists:
      self.thumbnails[self.currentGdalThumbnail2].loadTempFile(self.gdalPath2)
      self.thumbnails[self.currentGdalThumbnail2].restoreDefaultWorldFile() #Restore original coordinates
      self.updateStatus( self.currentGdalThumbnail2, "Finished" )
      QgsLogger.debug("Temporary file created")
    else:
      self.updateStatus( self.currentGdalThumbnail2, "Failed" )
      QgsLogger.debug("Failed to create temporary file")
    for i in range(len(self.thumbnails)):
      if not self.thumbnails[i].isFinishedRendering() and not(self.currentGdalThumbnail == i):
	self.currentGdalThumbnail2 = i
	self.createGdalThumbnails(self.currentGdalThumbnail2, 1)
	break
    
  def goToNext(self):
    if self.isGeoreferencing and self.currentThumbnailIndex + 1 < len(self.thumbnails):
      self.setCurrentThumbnail(self.currentThumbnailIndex+1)
      self.unloadTheResult()
      self.renderThumbnail(self.currentThumbnailIndex)
      
  def goToPrevious(self):
    if self.isGeoreferencing and self.currentThumbnailIndex > 0:
      self.setCurrentThumbnail(self.currentThumbnailIndex-1)
      self.unloadTheResult()
      self.renderThumbnail(self.currentThumbnailIndex)
      
  def selectTable(self):
    rows = self.ui.thumbnailTableWidget.selectedIndexes()
    if len(rows) > 0:
      self.setCurrentThumbnail(rows[0].row())
      if self.isGeoreferencing:
	self.unloadTheResult()
	self.renderThumbnail(self.currentThumbnailIndex)
    else:
      self.showMessage("Please first select a thumbnail from the table.", "warning")
    
  def setCurrentThumbnail(self, index):
    self.currentThumbnailIndex = index
    self.ui.thumbnailTableWidget.selectRow(self.currentThumbnailIndex)
    
  def selectRawPoint(self, point, button):
    if self.isGeoreferencing:
      if self.resultIsFocused:
	self.unloadTheResult()
	self.renderThumbnail(self.currentThumbnailIndex)
      else:
	self.rawPoint = QgsPoint(point)#self.transformCoordinates(point, self.referenceEpsg, self.thumbnails[ self.currentThumbnailIndex ].epsgNum())
	if self.rawMarker != None:
	  self.ui.canvas.scene().removeItem(self.rawMarker)
	self.rawMarker = QgsVertexMarker(self.ui.canvas)
	self.rawMarker.setCenter(point)
	self.rawMarker.setColor(QColor(0,255,0))
	self.rawMarker.setIconSize(5)
	self.rawMarker.setIconType(QgsVertexMarker.ICON_CROSS)
	self.rawMarker.setPenWidth(3)
	self.ui.canvas.refresh()
	self.hide()
	self.showMinimized()
      self.checkPoints()
      
  def checkPoints(self):
    if self.settings.value("AcceptPoints", QVariant(False)).toBool():
      if self.rawPoint != None and self.refPoint != None:
	self.acceptPoint()
      
  def acceptPoint(self):
    if len(self.thumbnails) > 0:
      self.thumbnails[ self.currentThumbnailIndex ].updateAttributes(self.refPoint, self.rawPoint)
      self.settings.setValue(self.thumbnails[ self.currentThumbnailIndex ].imagePath(), "Thumbnail")
      self.updateTable( self.currentThumbnailIndex, self.thumbnails[ self.currentThumbnailIndex ].hasSourceFile(), QString.number( self.refPoint.x(), 'f', 7 ), QString.number( self.refPoint.y(), 'f', 7 ), QString.number( self.thumbnails[ self.currentThumbnailIndex ].xOffset(), 'f', 7 ), QString.number( self.thumbnails[ self.currentThumbnailIndex ].yOffset(), 'f', 7 ) )
      if self.loadResult:
	self.hide()
	self.showMinimized()
	self.loadTheResult()
	self.resultIsFocused = True
	if self.currentThumbnailIndex < len(self.thumbnails)-1:
	  self.setCurrentThumbnail(self.currentThumbnailIndex+1)
      else:
	if self.currentThumbnailIndex < len(self.thumbnails)-1:
	  self.setCurrentThumbnail(self.currentThumbnailIndex+1)
	if self.refMarker != None:
	    self.iface.mapCanvas().scene().removeItem(self.refMarker)
	if self.rawMarker != None:
	    self.ui.canvas.scene().removeItem(self.rawMarker)
	self.renderThumbnail(self.currentThumbnailIndex)
	self.resultIsFocused = False
      self.rawPoint = None
      self.refPoint = None
    
  def selectReferencePoint(self, point, button):
    if self.isGeoreferencing:
      if self.resultIsFocused:
	self.unloadTheResult()
	self.renderThumbnail(self.currentThumbnailIndex)
	self.showNormal()
      else:
	self.refPoint = self.transformCoordinates(point, self.referenceEpsg, self.thumbnails[ self.currentThumbnailIndex ].epsgNum())
	if self.refMarker != None:
	  self.iface.mapCanvas().scene().removeItem(self.refMarker)
	self.refMarker = QgsVertexMarker(self.iface.mapCanvas())
	self.refMarker.setCenter(point)
	self.refMarker.setColor(QColor(0,255,0))
	self.refMarker.setIconSize(5)
	self.refMarker.setIconType(QgsVertexMarker.ICON_CROSS)
	self.refMarker.setPenWidth(3)
	self.refMarker.update()
	self.showNormal()
      self.checkPoints()
    
  def renderThumbnail(self, index):
    if index < len(self.thumbnails):
      layer = QgsRasterLayer( self.thumbnails[ index ].mGdalDatasetPath, self.thumbnails[ index ].mGdalDatasetPath )
      system = QgsCoordinateReferenceSystem()
      system.createFromEpsg(self.thumbnails[ index ].epsgNum())
      layer.setCrs(system)
      #self.iface.mapCanvas().setExtent(layer.extent())
      self.iface.mapCanvas().setExtent(self.thumbnails[ index ].referenceExtent())
      #self.previousThumbnailId = QgsMapLayerRegistry.instance().addMapLayer( layer, False ).getLayerID()
      QgsMapLayerRegistry.instance().addMapLayer( layer, False )
      layerSet = []
      mapCanvasLayer = QgsMapCanvasLayer(layer, True)
      layerSet.append(mapCanvasLayer)
      self.ui.canvas.setExtent(layer.extent())
      self.ui.canvas.setLayerSet(layerSet)
      width = self.ui.canvas.size().width()
      height = self.ui.canvas.size().height()
      self.ui.canvas.map().resize( QSize( width, height ) )
      self.ui.canvas.updateScale()
      self.ui.canvas.setVisible(True)
      self.ui.canvas.refresh()
    
  def createBackup(self, path):
    if self.settings.value("CreateBackup", QVariant(True)).toBool():
      shutil.copy2(path, path+".backup")
    
  def startWaitCursor(self):
    self.mainWindowCursor = self.iface.mainWindow().cursor()
    self.canvasCursor = self.iface.mapCanvas().cursor()
    self.layerCursor = self.iface.layerMenu().cursor()
    self.iface.mainWindow().setCursor(Qt.WaitCursor)
    self.iface.mapCanvas().setCursor(Qt.WaitCursor)
    self.iface.layerMenu().setCursor(Qt.WaitCursor)
    
  def stopWaitCursor(self):
    self.iface.mainWindow().setCursor(self.mainWindowCursor)
    self.iface.mapCanvas().setCursor(self.canvasCursor)
    self.iface.layerMenu().setCursor(self.layerCursor)
    
  def unloadTheResult(self):
    self.resultIsFocused = False
    QgsMapLayerRegistry.instance().removeMapLayer( self.resultLayerId )
    
  def loadTheResult(self):
    self.startWaitCursor()
    thumbnail = self.thumbnails[ self.currentThumbnailIndex ]
    info = QFileInfo( thumbnail.imagePath() )
    self.resultLayerPath = QDir.temp().absolutePath()+"/qgis_sumbandilasat/"+info.fileName()+"RESULT.jpg"

    self.resampleThumbnail(thumbnail, self.resultLayerPath)
    self.renderResult()
    self.stopWaitCursor()
   
  def resampleThumbnail(self, thumbnail, outputPath, makeSmaller = True):
    self.factor = 10
    d1 = gdal.Open(str(thumbnail.imagePath()))
    dt = d1.GetRasterBand(1).DataType
    nb = d1.RasterCount
    sx = d1.RasterXSize
    sy = d1.RasterYSize
    factorUsed = False
    if makeSmaller and (d1.RasterXSize > 2000 or d1.RasterYSize > 2000):
      sx = int(d1.RasterXSize / self.factor)
      sy = int(d1.RasterYSize / self.factor)
      factorUsed = True
    d2 = gdal.GetDriverByName('pcidsk').Create(str(outputPath)+'.pix', sx, sy, nb, dt)
    data = np.zeros([sy,sx])
    for b in range(1,nb+1):
      orig = d1.GetRasterBand(b).ReadAsArray()
      for y in range(0,sy):
	for x in range(0,sx):
	  if factorUsed:
	    data[y,x] = orig[y*self.factor, x*self.factor]
	  else:
	    data[y,x] = orig[y, x]
      d2.GetRasterBand(b).WriteArray(data)
    d2.FlushCache()
    d3 = gdal.GetDriverByName('jpeg').CreateCopy(str(outputPath), d2)
    d3.FlushCache()
    
  def renderResult(self):
    self.isAdding = True
    info = QFileInfo( self.resultLayerPath )
    thumbnail = self.thumbnails[ self.currentThumbnailIndex ]
    flip = False
    if thumbnail.hasDimSource:
      flip = True
    resultImage = Thumbnail(self.resultLayerPath, self.referenceCrs, "", flip)
    resultImage.setCorners(thumbnail.newTL, thumbnail.newTR, thumbnail.newBR, thumbnail.newBL)
    resultImage.setSize(thumbnail.columns()/ self.factor, thumbnail.rows()/ self.factor)
    resultImage.setEpsgNum(thumbnail.epsgNum())
    if thumbnail.hasDimSource:
      resultImage.setCrsDim(self.referenceCrs)
    elif thumbnail.hasWorldSource:
      resultImage.setCrsWorld(thumbnail.newTL, thumbnail.mSourcePath, self.referenceCrs)
    layer = QgsRasterLayer( self.resultLayerPath, info.completeBaseName() )
    layer.setCrs(self.referenceCrs)
    #layer.setTransparency(95)
    self.resultLayerId = QgsMapLayerRegistry.instance().addMapLayer( layer ).getLayerID()
    self.isAdding = False
    
  def transformCoordinates(self, point, srcEpsg, dstEpsg):
    ct = QgsCoordinateTransform()
    cs = QgsCoordinateReferenceSystem()
    cs.createFromEpsg(srcEpsg)
    ct.setSourceCrs(cs)
    ds = QgsCoordinateReferenceSystem()
    ds.createFromEpsg(dstEpsg)
    ct.setDestCRS(ds)
    return ct.transform(point)
    
  def setGeoreferencing(self):
    if self.ui.thumbnailTableWidget.rowCount() > 0:
      if self.thumbnails[ 0 ].isFinishedRendering():
	self.iface.mapCanvas().setMapTool(self.emitPoint)
	self.ui.canvas.setMapTool(self.emitRawPoint)
	if not self.isInitialized:
	  self.isInitialized = True
	  if self.isGeoreferencing:
	    #self.isGeoreferencing = False
	    self.iface.mapCanvas().unsetMapTool(self.emitPoint)
	    self.ui.messageWidget.hide()
	    self.ui.referenceComboBox.show()
	    self.referenceLayerIndex = -1
	  else:
	    self.referenceLayerIndex = self.ui.referenceComboBox.currentIndex()
	    self.isGeoreferencing = True
	    self.resultLayerId = ""
	    self.resultIsFocused = False
	    if self.selecedReferenceLayerId == "":
	      self.selectReferenceLayer(self.ui.referenceComboBox.currentText())
	    for layer in self.iface.legendInterface().layers(): #Hide all layers except the reference layer
	      if not self.selecedReferenceLayerId == layer.getLayerID():
		QgsMapLayerRegistry.instance().removeMapLayer( layer.getLayerID() )
	    self.ui.messageWidget.show()
	    self.ui.referenceComboBox.hide()
	    self.isAdding = True
	    if self.currentThumbnailIndex < 0:
	      index = 0
	      for thumbnail in self.thumbnails:
		if not thumbnail.wasGeoreferenced:
		  break
		index += 1
	      self.setCurrentThumbnail(index)
	    self.renderThumbnail(self.currentThumbnailIndex)
	    self.isAdding = False
	    #self.hide()
	    #self.showMinimized()
      else:
	#self.ui.actionSelectPoints.setChecked(False)
	pass
    
  def selectReferenceLayer(self, layer):
    if self.isEnabled():
      self.selecedReferenceLayerName = layer
      count = self.ui.referenceComboBox.count()
      layerId = ""
      for i in range(count):
	if self.ui.referenceComboBox.itemText( i ) == layer:
	  layerId = self.layerIds[ i ]
	  break
      if not layerId == "":
	self.isAdding = True
	self.selecedReferenceLayerId = layerId
	self.selecedReferenceLayer = QgsMapLayerRegistry.instance().mapLayer(self.selecedReferenceLayerId)
	self.isAdding = False
	if not self.selecedReferenceLayer == None:
	  self.ui.messageLabel.setText(self.selecedReferenceLayer.getLayerID() + " (" + self.selecedReferenceLayer.name() + ")")
	  self.referenceEpsg = self.selecedReferenceLayer.crs().epsg()
	  self.referenceCrs.createFromEpsg(self.referenceEpsg)
	  self.ui.crsLabel.setText(str(self.referenceCrs.description())+" ("+str(self.referenceEpsg)+")")
    
  def loadReferenceLayer(self):
    if not self.isAdding and self.referenceLayerIndex == -1:
      self.ui.referenceComboBox.clear()
      self.layerIds = []
      layerStrings = self.iface.legendInterface().layers()
      layers = []
      for layer in layerStrings:
	try:
	  layer.getLayerID()
	  layers.append( layer )
	  QgsLogger.debug( "Adding layer..." )
	except:
	  QgsLogger.debug( "Skipping layer key..." )
      if len(layers) < 1:
	self.selecedReferenceLayerId = ""
	self.ui.messageLabel.setText("No layers loaded")
	self.ui.messageWidget.show()
	self.ui.referenceComboBox.hide()
	self.ui.actionOpenDir.setEnabled(False)
	self.ui.actionAddThumbnail.setEnabled(False)
	self.ui.actionRemoveThumbnail.setEnabled(False)
      else:
	self.ui.actionOpenDir.setEnabled(True)
	self.ui.actionAddThumbnail.setEnabled(True)
	self.ui.actionRemoveThumbnail.setEnabled(True)
	self.ui.messageWidget.hide()
	self.ui.referenceComboBox.show()
	counter = 0
	index = 0
	for layer in layers:
	  item = layer.getLayerID() + " (" + layer.name() + ")"
	  self.layerIds.append( layer.getLayerID() )
	  self.ui.referenceComboBox.addItem( item )
	  counter += 1
	if self.selecedReferenceLayerId == "":
	  self.ui.referenceComboBox.setCurrentIndex(0)
	  self.selectReferenceLayer(self.ui.referenceComboBox.itemText(0))

  def updateTable(self, index, hasSource = True, pointX = "--", pointY = "--", offsetX = "--", offsetY = "--"):
    widget = QWidget()
    layout = QHBoxLayout(widget)
    layout.setContentsMargins(0, 0, 0, 0)
    layout.setSpacing(0)
    layout.setAlignment(Qt.AlignHCenter)
    if hasSource:
      checkBox = QCheckBox("")
      checkBox.setChecked(True)
      layout.addWidget(checkBox)
    else:
      checkBox = QCheckBox("")
      checkBox.setChecked(False)
      layout.addWidget(checkBox)
    self.ui.thumbnailTableWidget.setCellWidget(index, 1, widget)
    pointItem = QTableWidgetItem( "[" + pointX + ", " + pointY + "]" )
    self.ui.thumbnailTableWidget.setItem( index, 2, pointItem )
    offsetItem = QTableWidgetItem( "[" + offsetX + ", " + offsetY + "]" )
    self.ui.thumbnailTableWidget.setItem( index, 3, offsetItem )
    
  def updateStatus(self, index, status):
    bar = QProgressBar()
    bar.setMinimum(0)
    bar.setTextVisible(True)
    if status == "Queued" or status == "Failed":
      bar.setMaximum(100)
      bar.setValue(0)
      self.ui.thumbnailTableWidget.setCellWidget(index, 4, bar)
    elif status == "Started":
      bar.setMaximum(0)
      self.ui.thumbnailTableWidget.setCellWidget(index, 4, bar)
    elif status == "Finished":
      bar.setMaximum(100)
      bar.setValue(100)
      self.ui.thumbnailTableWidget.setCellWidget(index, 4, bar)

  def findFilesRecursively(self, paths, structure, fileTypes = ""):
    if fileTypes == "":
      fileTypes = "*"
    result = QStringList()
    more = QStringList()
    for i in range(len(paths)):
      mdir = QDir(paths[i])
      tmp = mdir.entryInfoList( fileTypes, QDir.Files )
      for j in range(len(tmp)):
	d = QDir(tmp[j].absoluteFilePath())
	if d.absolutePath().endsWith(".jpg"):
	  d.cdUp()
	  if structure == 1 and d.absolutePath().endsWith("/jpeg"):
	    more.append(tmp[j].fileName())
	  elif structure == 2:
	    more.append(tmp[j].fileName())
	  elif structure == 0:
	    more.append(tmp[j].fileName())
      for j in range(len(more)):
	contains = False
	for k in range(len(result)):
	  if result[k].endsWith(more[j]):
	    contains = True
	if not contains:
	  result.append( paths[i] + "/" + more[j] )
      more = mdir.entryList( QDir.Dirs ).filter( QRegExp( "[^.]" ) )
      for j in range(len(more)):
	more[j] = paths[i] + "/" + more[j]
      more = self.findFilesRecursively( more, structure, fileTypes )
      for j in range(len(more)):
	if not result.contains(more[j]) and not result.contains(more[j]):
	  result.append( more[j] )
    return result

  def changeLastOpenDir(self, path):
    self.settings.setValue("PreviousDirectory", path)
    self.defaultDir = path

  def openDirectory(self):
    # Use pdb for debugging
    import pdb
    # These lines allow you to set a breakpoint in the app
    pyqtRemoveInputHook()
    pdb.set_trace()

    mlist = QStringList()
    mdir = QFileDialog.getExistingDirectory( self, "Open Thumbnail Directory", self.defaultDir, QFileDialog.ShowDirsOnly )
    if len(mdir) > 0:
      self.changeLastOpenDir(mdir)
      mlist.append(mdir)
      structure = int(self.settings.value("StructureChoice", QVariant(0)).toString())
      names = self.findFilesRecursively(mlist, structure, "*.jpg")
      self.ui.messageLabel.setText(names.join(" : "))
      images = []
      for name in names:
	if not self.settings.value(name).toString() == "Thumbnail":
	  images.append(name)
      if len(images) > 0:
	self.loadThumbnails(images)

  def openThumbnails(self):
    fileNames = QFileDialog.getOpenFileNames( self, "Open Thumbnails", self.defaultDir, "JPEG Thumbnails (*.jpg *.jpeg)" )
    if len(fileNames) > 0:
      info = QFileInfo(fileNames[0])
      self.changeLastOpenDir(info.dir().absolutePath())
      self.loadThumbnails(fileNames)

  def loadThumbnails(self, fileNames):
    counter = self.ui.thumbnailTableWidget.rowCount()
    for fileName in fileNames:
      alreadyLoaded = False
      for thumbnail in self.thumbnails:
	if thumbnail.imagePath() == fileName:
	  alreadyLoaded = True
	  break
      if not alreadyLoaded:
	info = QFileInfo( fileName )
	flip = False
	structure = int(self.settings.value("StructureChoice", QVariant(0)).toString())
	if structure == 1:
	  flip = True
	if structure == 0:
	  self.sourcePath = "AUTO"
	thumbnail = Thumbnail( fileName, self.referenceCrs, self.sourcePath, flip )
	self.createBackup(thumbnail.mSourcePath)
	if thumbnail.hasDimSource or (thumbnail.convertCoordinates() and not thumbnail.invalidCoordinates):
	  self.ui.thumbnailTableWidget.setRowCount( counter + 1 )
	  self.thumbnails.append( thumbnail )
	  nameItem = QTableWidgetItem( info.fileName() )
	  self.ui.thumbnailTableWidget.setItem( counter, 0, nameItem )
	  self.updateTable( counter, thumbnail.hasSourceFile() )
	  self.updateStatus( counter, "Queued" )
	  counter += 1
	  QgsLogger.debug( "Thumbnail Added "+thumbnail.mSourcePath, 1, "georeferencer.py", "SumbandilaSat Georeferencer")
	else:
	  self.showMessage("The world file from this thumbnail cannot be processed.\nThe thumbnail will be removed from the list.")
    
    for i in range(len(self.thumbnails)):
      if not self.thumbnails[i].isFinishedRendering() and not(self.currentGdalThumbnail2 == i):
	self.currentGdalThumbnail = i
	if not self.isRendering:
	  self.createGdalThumbnails(self.currentGdalThumbnail, 0)
	break
    if not self.currentGdalThumbnail == len(self.thumbnails) - 1:
      for i in range(len(self.thumbnails)):
	if not self.thumbnails[i].isFinishedRendering() and not(self.currentGdalThumbnail == i):
	  self.currentGdalThumbnail2 = i
	  if not self.isRendering2:
	    self.createGdalThumbnails(self.currentGdalThumbnail2, 1)
	  break
      
  def removeThumbnails(self):
    if self.isRendering or self.isRendering2:
      self.showMessage("Please wait until all thumbnails were rendered.", "warning")
    elif len(self.thumbnails) > 0 and len(self.ui.thumbnailTableWidget.selectedIndexes()) > 0:
      self.thumbnails.pop(self.ui.thumbnailTableWidget.currentRow()-1)
      self.ui.thumbnailTableWidget.removeRow(self.ui.thumbnailTableWidget.currentRow())
