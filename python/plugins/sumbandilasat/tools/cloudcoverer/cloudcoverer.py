# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from qgis.analysis import *
from settings import *
from ui_cloudcoverer import Ui_CloudCoverer
import gdal
from gdalconst import *

class CloudCovererWindow(QDialog):

  def __init__(self, iface):
    QMainWindow.__init__( self )
    self.ui = Ui_CloudCoverer()
    self.iface = iface
    self.ui.setupUi( self )
    self.layers = []
    self.imageId = -1
    
    d = QDir(QDir.temp().absolutePath()+"/qgis_sumbandilasat")
    if not d.exists():
      d.mkdir(QDir.temp().absolutePath()+"/qgis_sumbandilasat")
    self.analyzer = None
    self.adjustSlider()
    
    self.sliderPressed = False
    self.connect(self.ui.slider, SIGNAL("valueChanged(int)"), self.update)
    self.connect(self.ui.slider, SIGNAL("sliderPressed()"), self.pressSlider)
    self.connect(self.ui.slider, SIGNAL("sliderReleased()"), self.releaseSlider)
    self.connect(self.ui.openButton, SIGNAL("clicked()"), self.selectImage)
    self.connect(self.ui.settingsButton, SIGNAL("clicked()"), self.openSettings)
    self.connect(self.ui.lineEdit, SIGNAL("textChanged(const QString&)"), self.changeFile)
    self.connect(self.ui.buttonBox.button(QDialogButtonBox.Save), SIGNAL("clicked()"), self.saveToShp)
        
    self.xDevider = 10
    self.yDevider = 10
    self.bandCount = 0
    
    self.settings = QSettings("QuantumGIS", "SumbandilaSat CloudCoverer")
    
  def setImageId(self, idVal):
    self.imageId = idVal
    
  #Displays a message to the user
  def showMessage(self, string, type = "info"):
    msgbox = QMessageBox(self)
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
    
  def openSettings(self):
    self.dialog = SettingsDialog(self, self.settings)
    self.dialog.setBands(self.bandCount)
    self.dialog.show()
    
  def changeFile(self, path):
    self.ui.lineEdit.setText(path)
    f = QFile(path)
    d = QDir(path)
    if f.exists() and not d.exists(): #make sure the path is not a dir, otherwise GDAL has problems
      newPath = self.resample(path)
      self.analyzer = QgsCloudAnalyzer(newPath)
      self.layers = []
      self.openImage(newPath)
      self.adjustSlider()
    
  def selectImage(self):
    if int(self.settings.value("DirectoryChoice", QVariant(0)).toString()) == 0:
      path = QFileDialog.getOpenFileName(self, "Open Image", "")
    else:
      path = QFileDialog.getOpenFileName(self, "Open Image", self.settings.value("DirectoryPath", QVariant("")).toString())
    if path != "":
      if int(self.settings.value("DirectoryChoice", QVariant(0)).toString()) == 1:
        self.settings.setValue("DirectoryPath", path)
      self.ui.lineEdit.setText(path)
    
  def resample(self, path):
    myFileName = QDir.temp().absolutePath()+QDir.separator()+"cloudy.tif"
    #self.showMessage(myFileName)
    f = QFile( myFileName )
    if f.exists():
      f.remove()
    dataset = gdal.Open(str(path), GA_ReadOnly)
    if dataset is not None:
      process = QProcess(self)
      self.connect(process, SIGNAL("finished(int, QProcess::ExitStatus)"), self.processFinished)
      self.finishedResampling = False
      width = dataset.RasterXSize
      height = dataset.RasterYSize
      self.bandCount = dataset.RasterCount
      while width > 1000 and height > 1000:
        width /= self.xDevider
        height /= self.yDevider
      process.start('gdal_translate -of GTiff -co PROFILE=BASELINE -outsize '+str(width)+' '+str(height)+' '+path+' '+ myFileName)
      process.waitForStarted()
      process.waitForFinished()
      while not self.finishedResampling:
        pass
      return myFileName
    return ""
    
  def processFinished(self, a, b):
    self.finishedResampling = True
    
  def adjustSlider(self):
    if self.analyzer != None:
      self.ui.slider.setMinimum(self.analyzer.lowestValue())
      self.ui.slider.setMaximum(self.analyzer.highestValue())
      self.ui.slider.setValue(self.analyzer.highestValue()*0.95)
      self.ui.spinBox.setMinimum(self.analyzer.lowestValue())
      self.ui.spinBox.setMaximum(self.analyzer.highestValue())
      self.ui.spinBox.setValue(self.analyzer.highestValue()*0.95)
  
  def pressSlider(self):
    self.sliderPressed = True
    
  def releaseSlider(self):
    if self.analyzer != None:
      self.analyzer.setThreshold(self.ui.slider.value())
      self.reanalyze()
      self.sliderPressed = False
    
  def update(self, value = 0):
    if not self.sliderPressed:
      self.analyzer.setThreshold(self.ui.slider.value())
      self.reanalyze()
    
  def openImage(self, path):
    layer = QgsRasterLayer( path )
    layer.setContrastEnhancementAlgorithm( QgsContrastEnhancement.StretchToMinimumMaximum, False )
    self.iface.mapCanvas().setExtent(layer.extent())
    QgsMapLayerRegistry.instance().addMapLayer( layer, False )
    mapCanvasLayer = QgsMapCanvasLayer(layer, True)
    self.layers.append(mapCanvasLayer)
    self.ui.canvas.setExtent(layer.extent())
    self.ui.canvas.setLayerSet(self.layers)
    self.ui.canvas.setVisible(True)
    self.ui.canvas.refresh()
    
  def openMask(self, mask):
    self.ui.percentage.setText(str(self.analyzer.percentage())+"%")
    QObject.emit(self, SIGNAL("changed(int, int)"), self.analyzer.percentage(), self.imageId)
    if len(self.layers) > 1:
      self.layers.pop(0)
    QgsMapLayerRegistry.instance().addMapLayer(mask, False)
    self.layers.insert(0, QgsMapCanvasLayer(mask, True))
    #self.ui.canvas.setExtent(mask.extent())
    self.ui.canvas.setLayerSet(self.layers)
    self.ui.canvas.setVisible(True)
    self.ui.canvas.refresh()
    
  def reanalyze(self):
    band = 1
    bandChoice = int(self.settings.value("BandUse", QVariant(0)).toString())
    if (bandChoice+1) <= self.bandCount and (bandChoice+1) > 0:
      band = bandChoice+1
    self.analyzer.setBandUse(band)
    size = float(self.settings.value("UnitSize", QVariant(0.0)).toString())
    if size <= 0.0:
      self.settings.setValue("UnitSize", 0.1)
      size = 0.1
    self.analyzer.setUnitSize(size)
    self.analyzer.setCloudColor(QColor(self.settings.value("CloudColor", QVariant(""))))
    mask = self.analyzer.analyze()
    maskLayer = mask.vectorLayer()
    self.openMask(maskLayer)
    myCrs = QgsCoordinateReferenceSystem("CRS:84")
    myPath = QDir.tempPath() + QDir.separator() + "clouds.shp"

  def saveToShp(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("cloudMasker/lastOutputDir").toString()
    fileName = QFileDialog.getSaveFileName(self, "Save as shapefile", myLastDir,"Shapefile (*.shp)" )
    if fileName != "":
      mySettings.setValue("cloudMasker/lastOutputDir", QFileInfo( fileName ).absolutePath())
      mask = self.analyzer.analyze()
      maskLayer = mask.vectorLayer()
      if maskLayer:
        myCrs = QgsCoordinateReferenceSystem("CRS:84")
        error = QgsVectorFileWriter.writeAsVectorFormat(mask.vectorLayer(), fileName,"UTF-8", myCrs)
        self.iface.addVectorLayer(fileName, "clouds", "ogr")
      else:
        showMessage("There was a problem writing the file to disk")
    
    
