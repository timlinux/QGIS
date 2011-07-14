# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui, QtWebKit, QtNetwork
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtWebKit import *
from PyQt4.QtNetwork import *
from qgis.core import *
from qgis.gui import *
import os.path
import os
import shutil
from math import *
import osr
import osgeo.gdal as gdal
from extractionlayer import *

dimension = 600

class Extent():
  
  def __init__(self, xmin = 0, ymin = 0, xmax = 0, ymax = 0):
    self.xmin = xmin
    self.ymin = ymin
    self.xmax = xmax
    self.ymax = ymax
    
  def xMinimum(self):
    return self.xmin
    
  def yMinimum(self):
    return self.ymin
    
  def xMaximum(self):
    return self.xmax
    
  def yMaximum(self):
    return self.ymax

class Tile():
  
  def __init__(self, path, extent, count):
    self.path = path
    self.extent = extent
    self.count = count

class ImageExtractor(QObject):

  def __init__(self):
    QObject.__init__( self )
    self.webView = QWebView()
    self.webView.setFixedWidth(dimension)
    self.webView.setFixedHeight(dimension)
    self.manager = QNetworkAccessManager()
    self.page = QWebPage()
    self.pageTest = QWebPage()
    QObject.connect(self.webView, SIGNAL("loadFinished(bool)"), self.writeImage)
    
    self.path = ""
    self.count = 0
    self.clearTemp()
    
    self.outputPath = ""
    self.extent = Extent()
    self.extractionStarted = False
    self.tiles = []
    self.tilePaths = []
    self.totalTileCount = 0
    self.skipLevel = 99
    self.redownloadLevel = 1
    self.cutOverhead = True
    
    self.webViewTest = None
    self.maxDevider = 1
    self.maxExtent = Extent(0,0,0,0)
    
    self.layerName = "GoogleSatellite"
    self.layer = ExtractionLayer()
    
    self.proxy = QNetworkProxy()

  def setProxy(self, proxy):
    self.proxy = proxy
    self.manager.setProxy(self.proxy)
    self.page.setNetworkAccessManager(self.manager)
    self.pageTest.setNetworkAccessManager(self.manager)
    self.webView.setPage(self.page)

  def calculateMaximumLevel(self):
    self.emit(SIGNAL("logged(QString)"), "Current zoom level at "+str(self.maxDevider)+" ...")
    self.emit(SIGNAL("progressed(double, int, int)"), 0, 0, 0)
    xDiffrence = (self.maxExtent.xMaximum() - self.maxExtent.xMinimum())/self.maxDevider
    yDiffrence = (self.maxExtent.yMaximum() - self.maxExtent.yMinimum())/self.maxDevider
    newMinX = self.maxExtent.xMinimum()+(xDiffrence*self.maxDevider)
    newMinY = self.maxExtent.yMinimum()+(yDiffrence*self.maxDevider)
    newMaxX = newMinX+xDiffrence
    newMaxY = newMinY+yDiffrence
    newExtent = Extent(newMinX, newMinY, newMaxX, newMaxY)
    self.webViewTest.setHtml(self.layer.get(newExtent, self.layerName))
    
  def evaluateLevel(self, ok):
    page = self.webViewTest.page()
    image = QImage(page.viewportSize(), QImage.Format_RGB32)
    painter = QPainter(image)
    page.mainFrame().render(painter)
    painter.end()
    badPixelCount = 0
    
    if QString(self.layerName).startsWith("Google"):
      color = QColor(229, 227, 223)
    elif QString(self.layerName).startsWith("Yahoo"):
      color = QColor(204,204,204)
    else:
      color = QColor(0,0,0)
   
    for i in range(dimension):
      for j in range(dimension):
        if QColor(image.pixel(i, j)) == color:
	  badPixelCount += 1
	 
    if badPixelCount > (dimension*dimension*0.7):
      self.maxDevider -= 1
      if self.maxDevider < 1:
	self.maxDevider = 1
      QObject.disconnect(self.webViewTest, SIGNAL("loadFinished(bool)"), self.evaluateLevel)
      self.emit(SIGNAL("logged(QString)"), "Finished zoom level determination. Maximum level set at "+str(self.maxDevider)+" ...")
      self.webViewTest = None
      self.startExtraction(self.maxExtent, self.maxDevider)
    else:
      self.maxDevider += 1
      self.calculateMaximumLevel()

  def stop(self):
    if self.webViewTest != None:
      try:
	self.webViewTest.stop()
	QObject.disconnect(self.webViewTest, SIGNAL("loadFinished(bool)"), self.evaluateLevel)
      except:
	pass
    self.webView.stop()
    QObject.disconnect(self.webView, SIGNAL("loadFinished(bool)"), self.writeImage)
    self.clearTemp()
    self.emit(SIGNAL("progressed(double, int, int)"), 0, 0, 0)
    self.emit(SIGNAL("logged(QString)"), "The extraction was canceled.")

  def clearTemp(self):
    self.path = QDir.tempPath()+QDir.separator()+"leechy"
    if os.path.exists(self.path):
      shutil.rmtree(str(self.path))
    os.mkdir((self.path))
    self.path += QDir.separator()+"image.bmp"

  def extractArea(self, extent, devider = 2):
    self.maxExtent = Extent(extent.xMinimum(), extent.yMinimum(), extent.xMaximum(), extent.yMaximum())
    if devider < 0: #Test the maximum zoom level, else just continue with the user-defined level
      self.emit(SIGNAL("logged(QString)"), "Starting to determine the maximum zoom level ...")
      self.webViewTest = QWebView()
      self.webViewTest.setFixedWidth(dimension)
      self.webViewTest.setFixedHeight(dimension)
      self.webViewTest.setPage(self.pageTest)
      QObject.connect(self.webViewTest, SIGNAL("loadFinished(bool)"), self.evaluateLevel)
      self.maxDevider = 1
      self.calculateMaximumLevel()
    else:
      self.startExtraction(extent, devider)
      
  def startExtraction(self, extent, devider):
    self.totalTileCount = 0
    self.clearTemp()
    xDiffrence = (extent.xMaximum() - extent.xMinimum())/devider
    yDiffrence = (extent.yMaximum() - extent.yMinimum())/devider
    for i in range(devider):
      for j in range(devider):
	newMinX = extent.xMinimum()+(xDiffrence*j)
	newMinY = extent.yMinimum()+(yDiffrence*i)
	newMaxX = newMinX+xDiffrence
	newMaxY = newMinY+yDiffrence
	newExtent = Extent(newMinX, newMinY, newMaxX, newMaxY)
	self.tiles.append(Tile(QString(self.path).insert(len(self.path)-4, "_"+str(j)+"_"+str(i)), newExtent, 1))
	self.totalTileCount += 1
    self.emit(SIGNAL("logged(QString)"), "Starting with the tile extraction ...")
    self.extractNext()
	
  def extractNext(self):
    if(len(self.tiles) > 0):
      index = QString(self.tiles[0].path).lastIndexOf(QDir.separator())
      string = QString(self.tiles[0].path).right(QString(self.tiles[0].path).length()-index-7)
      column = string.left(string.indexOf("_"))
      string = string.right(string.length()-string.indexOf("_")-1)
      row = string.left(string.indexOf("."))
      msg = "Extracting tile for row "+str(int(row)+1)+" and column "+str(int(column)+1)+" ..."
      self.emit(SIGNAL("logged(QString)"), msg)
      self.extractImage(self.tiles[0].path, self.tiles[0].extent, self.tiles[0].count)
      self.tiles.pop(0)
      currentTile = self.totalTileCount-len(self.tiles)
      if currentTile < 0:
	currentTile = 0
      if self.totalTileCount != 0:
	progress = (float(currentTile-1)/self.totalTileCount)*97.0
      else:
	progress = 0
      self.emit(SIGNAL("progressed(double, int, int)"), progress, currentTile, self.totalTileCount)
    else:
      self.emit(SIGNAL("logged(QString)"), "Finished with tile extraction ...")
      self.emit(SIGNAL("logged(QString)"), "Starting with image reassembly ...")
      self.reassembleImage(self.outputPath)

  def extractImage(self, path, extent, count):
    self.path = path
    self.extent = extent
    self.count = count
    self.extractionStarted = True
    self.webView.setHtml(self.layer.get(extent, self.layerName))
    
  def writeImage(self, ok):
    page = self.webView.page()
    xmin = page.mainFrame().evaluateJavaScript("map.getExtent().left").toDouble()[0]
    ymin = page.mainFrame().evaluateJavaScript("map.getExtent().top").toDouble()[0]
    xmax = page.mainFrame().evaluateJavaScript("map.getExtent().right").toDouble()[0]
    ymax = page.mainFrame().evaluateJavaScript("map.getExtent().bottom").toDouble()[0]
    self.extent = Extent(xmin, ymin, xmax, ymax)
    page.setViewportSize(QSize(dimension,dimension))
    image = QImage(page.viewportSize(), QImage.Format_RGB32)
    painter = QPainter(image)
    page.mainFrame().render(painter)
    painter.end()
    image.save(self.path)
    
    if self.checkQuality(self.path, self.extent, self.count):
      
      ct = QgsCoordinateTransform()
      cs1 = QgsCoordinateReferenceSystem()
      cs1.createFromEpsg(900913)
      cs2 = QgsCoordinateReferenceSystem()
      cs2.createFromEpsg(4326)
      
      ct.setSourceCrs(cs1)
      ct.setDestCRS(cs2)
      
      point1 = ct.transform(QgsPoint(self.extent.xMinimum(), self.extent.yMinimum()))
      point2 = ct.transform(QgsPoint(self.extent.xMaximum(), self.extent.yMaximum()))
	
      pixelX = (point2.x()-point1.x())/(dimension*1.0)
      pixelY = (point2.y()-point1.y())/(dimension*1.0)
      
      driver = gdal.GetDriverByName("GTiff")
      src_ds = gdal.Open(str(self.path))
      dst_ds = driver.CreateCopy(str(QString(self.path).left(len(self.path)-3)+"tif"), src_ds, 0 )
      dst_ds.SetGeoTransform( [ point1.x(), pixelX, 0.0, point1.y(), 0.0, pixelY] )
      srs = osr.SpatialReference()
      srs.SetFromUserInput('EPSG:4326')
      dst_ds.SetProjection(srs.ExportToWkt())
      dst_ds = None
      src_ds = None
      os.remove(self.path)
      
      self.tilePaths.append(self.path)
    self.extractNext()
    
  def reassembleImage(self, path):
    process = QProcess(self)
    self.outputPath = path
    if os.path.exists(self.outputPath):
      os.remove(self.outputPath)
    QObject.connect(process, SIGNAL("finished(int, QProcess::ExitStatus)"), self.checkGeneration)
    tiles = ""
    for tile in self.tilePaths:
      tiles += str(QString(tile).left(len(tile)-3)+"tif")+" "
    if len(self.tilePaths) == 0:
      self.emit(SIGNAL("logged(QString)"), "No tiles are appropriate for reassembly. No output image will be generated ...")
    command = "python "+os.path.dirname( __file__ ).replace("\\", "/")+"/merger.py "
    command2 = ""
    if self.cutOverhead:
      ct = QgsCoordinateTransform()
      cs1 = QgsCoordinateReferenceSystem()
      cs1.createFromEpsg(900913)
      cs2 = QgsCoordinateReferenceSystem()
      cs2.createFromEpsg(4326)
	
      ct.setSourceCrs(cs1)
      ct.setDestCRS(cs2)
	
      point1 = ct.transform(QgsPoint(self.maxExtent.xMinimum(), self.maxExtent.yMinimum()))
      point2 = ct.transform(QgsPoint(self.maxExtent.xMaximum(), self.maxExtent.yMaximum()))
      command2 += "-ul_lr "+str(point1.x())+" "+str(point1.y())+" "+str(point2.x())+" "+str(point2.y())+" "
    self.extentlessCommand = command+"-o "+path+" "+tiles
    QgsLogger.debug(command+command2+"-o "+path+" "+tiles)
    process.start(command+command2+"-o "+path+" "+tiles)
    
  def checkGeneration(self, exitCode = 0, status = QProcess.NormalExit): #Check if the extents where inside the tiles. If not, no image will be generated and we have to try it without the cutoffs
    if not os.path.exists(self.outputPath):
      process = QProcess(self)
      QObject.connect(process, SIGNAL("finished(int, QProcess::ExitStatus)"), self.setFinished)
      process.start(self.extentlessCommand)
    else:
      self.setFinished()
    
  def setFinished(self, exitCode = 0, status = QProcess.NormalExit):
    self.clearTemp()
    self.emit(SIGNAL("logged(QString)"), "Finished with image reassembly.")
    self.emit(SIGNAL("progressed(double, int, int)"), 100, self.totalTileCount, self.totalTileCount)
      
  def checkQuality(self, path, extent, count):
    image = QImage(path)
    badPixelCount = 0
    for i in range(dimension):
      for j in range(dimension):
	if QColor(image.pixel(i, j)) == Qt.white:
	  badPixelCount += 1
    if count > 5:
      self.emit(SIGNAL("logged(QString)"), "The tile was downloaded 5 times. Skipping image ...")
      return True
    elif badPixelCount > (dimension*dimension*(self.skipLevel/100.0)):
      self.emit(SIGNAL("logged(QString)"), "More than "+str(self.skipLevel)+"% of the pixels are bad. Skipping image ...")
      return False
    elif badPixelCount > (dimension*dimension*(self.redownloadLevel/100.0)):
      self.emit(SIGNAL("logged(QString)"), "More than "+str(self.redownloadLevel)+"% of the pixels are bad. Re-downloading image ...")
      self.tiles.append(Tile(path, extent, count+1))
      return False
    return True