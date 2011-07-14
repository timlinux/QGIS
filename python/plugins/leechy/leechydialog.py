# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui, QtNetwork
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *
from qgis.core import *
from qgis.gui import *
from ui_leechydialog import Ui_LeechyDialog
from imageextractor import *
from helpdialog import *

class LeechyDialog(QDialog):

  def __init__(self, iface, version = "1.0.0.0"):
    QDialog.__init__(self)
    self.ui = Ui_LeechyDialog()
    self.iface = iface
    self.version = version
    self.ui.setupUi(self)
    
    QObject.connect(self.ui.outputButton, SIGNAL("clicked()"), self.selectFile)
    QObject.connect(self.ui.maxRadioButton, SIGNAL("clicked()"), self.changeRadioSelection)
    QObject.connect(self.ui.manualRadioButton, SIGNAL("clicked()"), self.changeRadioSelection)
    QObject.connect(self.ui.redownloadSlider, SIGNAL("valueChanged(int)"), self.changeRedownloadValue)
    QObject.connect(self.ui.skipSlider, SIGNAL("valueChanged(int)"), self.changeSkipValue)
    QObject.connect(self.ui.manualSpinBox, SIGNAL("valueChanged(int)"), self.changeLevel)
    QObject.connect(self.ui.manualSpinBox, SIGNAL("editingFinished()"), self.changeLevel)
    QObject.connect(self.ui.canvasButton, SIGNAL("clicked()"), self.getCanvasExtent)
    QObject.connect(self.ui.sourceComboBox, SIGNAL("currentIndexChanged(int)"), self.testConnection)
    
    QObject.connect(self.ui.backButton, SIGNAL("clicked()"), self.goBack)
    QObject.connect(self.ui.nextButton, SIGNAL("clicked()"), self.goNext)
    QObject.connect(self.ui.startButton, SIGNAL("clicked()"), self.start)
    QObject.connect(self.ui.stopButton, SIGNAL("clicked()"), self.stop)
    QObject.connect(self.ui.helpButton, SIGNAL("clicked()"), self.help)
    QObject.connect(self.ui.tabWidget, SIGNAL("currentChanged(int)"), self.changeTab)
    
    self.ui.availableWidget.hide()
    self.ui.notAvailableWidget.hide()
    self.ui.busyWidget.hide()
    self.networkManager = QNetworkAccessManager()
    self.timer = QTimer()
    self.timer.setSingleShot(True)
    self.timer.setInterval(7000)
    self.timing = False
    QObject.connect(self.timer, SIGNAL("timeout()"), self.receivedReply)
    QObject.connect(self.networkManager, SIGNAL("finished(QNetworkReply*)"), self.receivedReply)
    
    self.changeTab()
    self.settings = QSettings("QuantumGIS", "Leechy")
    self.loadSettings()
    self.changeRadioSelection()
    self.proxy = QNetworkProxy()
    self.testConnection()
    
  def setRadioVisibility(self):
    i = self.ui.sourceComboBox.currentIndex()
    if i == 3 or i == 4 or i == 6 or i == 7:
      self.ui.maxRadioButton.show()
      self.ui.manualRadioButton.show()
    else:
      self.ui.maxRadioButton.hide()
      self.ui.manualRadioButton.setChecked(True)
      self.ui.manualRadioButton.hide()
      self.ui.manualSpinBox.show()
    
  def testConnection(self, index = -1):
    url = ""
    if index == 0:
      url = "http://tile.openstreetmap.org"
    elif index == 1 or index == 2 or index == 3 or index == 4:
      url = "http://maps.google.com"
    else:
      url = "http://maps.yahoo.com"
    self.showConnection(3)
    self.timer.start()
    self.timing = True
    self.networkManager.get(QNetworkRequest(QUrl(url)))
    self.setRadioVisibility()
      
  def receivedReply(self, reply = None):
    if self.timing:
      if reply == None:
	self.showConnection(2)
      else:
	if reply.error() == QNetworkReply.NoError:
	  self.showConnection(1)
	else:
	  self.showConnection(2)
      
  def showConnection(self, item = 1):
    self.timing = False
    self.ui.availableWidget.hide()
    self.ui.notAvailableWidget.hide()
    self.ui.busyWidget.hide()
    if item == 1:
      self.ui.availableWidget.show()
    elif item == 2:
      self.ui.notAvailableWidget.show()
    elif item == 3:
      self.ui.busyWidget.show()
    
  def closeEvent(self, event):
    self.saveSettings()
    QDialog.closeEvent(self, event)
    
  def saveSettings(self):
    self.settings.setValue("source", self.ui.sourceComboBox.currentIndex())
    self.settings.setValue("output", self.ui.outputLineEdit.text())
    self.settings.setValue("minx", self.ui.minX.text())
    self.settings.setValue("miny", self.ui.minY.text())
    self.settings.setValue("maxx", self.ui.maxX.text())
    self.settings.setValue("maxy", self.ui.maxY.text())
    self.settings.setValue("levelselection", self.ui.maxRadioButton.isChecked())
    self.settings.setValue("levelvalue", self.ui.manualSpinBox.value())
    self.settings.setValue("redownload", self.ui.redownloadSlider.value())
    self.settings.setValue("skip", self.ui.skipSlider.value())
    self.settings.setValue("overhead", self.ui.overHeadCheckBox.isChecked())
    self.settings.setValue("proxy", self.ui.proxyCheckBox.isChecked())
    self.settings.setValue("loadimage", self.ui.loadCheckBox.isChecked())
    
  def loadSettings(self):
    self.ui.sourceComboBox.setCurrentIndex(int(self.settings.value("source", QVariant(0)).toString()))
    self.ui.outputLineEdit.setText(self.settings.value("output", QVariant("")).toString())
    self.ui.minX.setText(self.settings.value("minx", QVariant("")).toString())
    self.ui.minY.setText(self.settings.value("miny", QVariant("")).toString())
    self.ui.maxX.setText(self.settings.value("maxx", QVariant("")).toString())
    self.ui.maxY.setText(self.settings.value("maxy", QVariant("")).toString())
    self.ui.maxRadioButton.setChecked(self.settings.value("levelselection", QVariant(False)).toBool())
    self.ui.manualSpinBox.setValue(int(self.settings.value("levelvalue", QVariant(1)).toString()))
    self.ui.redownloadSlider.setValue(int(self.settings.value("redownload", QVariant(5)).toString()))
    self.ui.skipSlider.setValue(int(self.settings.value("skip", QVariant(99)).toString()))
    self.ui.overHeadCheckBox.setChecked(self.settings.value("overhead", QVariant(True)).toBool())
    self.ui.proxyCheckBox.setChecked(self.settings.value("proxy", QVariant(False)).toBool())
    self.ui.loadCheckBox.setChecked(self.settings.value("loadimage", QVariant(False)).toBool())
    
  def getCanvasExtent(self):
    ct = QgsCoordinateTransform()
    cs1 = self.iface.mapCanvas().mapRenderer().destinationSrs()
    cs2 = QgsCoordinateReferenceSystem()
    cs2.createFromEpsg(900913)
      
    ct.setSourceCrs(cs1)
    ct.setDestCRS(cs2)
      
    point1 = ct.transform(QgsPoint(self.iface.mapCanvas().extent().xMinimum(), self.iface.mapCanvas().extent().yMinimum()))
    point2 = ct.transform(QgsPoint(self.iface.mapCanvas().extent().xMaximum(), self.iface.mapCanvas().extent().yMaximum()))
    
    self.ui.minX.setText(str(point1.x()))
    self.ui.minY.setText(str(point1.y()))
    self.ui.maxX.setText(str(point2.x()))
    self.ui.maxY.setText(str(point2.y()))
    
  def help(self):
    self.helpDialog = HelpDialog(self, self.version)
    self.helpDialog.show()
    
  def changeRedownloadValue(self, value):
    self.ui.redownloadLabel.setText(str(value)+"%")
    
  def changeSkipValue(self, value):
    self.ui.skipLabel.setText(str(value)+"%")
    
  def changeRadioSelection(self):
    if self.ui.maxRadioButton.isChecked():
      self.ui.manualSpinBox.hide()
    else:
      self.ui.manualSpinBox.show()
    self.changeLevel()
    
  def selectFile(self):
    fileName = QFileDialog.getSaveFileName(self, "Save Output Image")
    if fileName != "":
      self.ui.outputLineEdit.setText(fileName)
    
  def changeTab(self, i = 0):
    index = self.ui.tabWidget.currentIndex()
    self.ui.startButton.hide()
    self.ui.stopButton.hide()
    self.ui.backButton.hide()
    self.ui.nextButton.hide()
    if index == 0:
      self.ui.nextButton.show()
    elif index == 4:
      self.ui.backButton.show()
      self.ui.startButton.show()
    else:
      self.ui.backButton.show()
      self.ui.nextButton.show()
      
  def goBack(self):
    if self.ui.tabWidget.currentIndex() > 0:
      self.ui.tabWidget.setCurrentIndex(self.ui.tabWidget.currentIndex()-1)
      
  def goNext(self):
    if self.ui.tabWidget.currentIndex() < 4:
      self.ui.tabWidget.setCurrentIndex(self.ui.tabWidget.currentIndex()+1)
      
  def changeLevel(self, val = 0):
    if self.ui.manualRadioButton.isChecked():
      value = self.ui.manualSpinBox.value()
    else:
      value = 0
    self.ui.progressLabel.setText("Processing tile 0 of "+str(value*value))
    
  def appendLog(self, string):
    self.ui.log.append(string)
    
  def clearLog(self):
    self.ui.log.clear()
      
  def progress(self, progress, currentTile, maxTile):
    self.ui.progressBar.setValue(progress)
    self.ui.progressLabel.setText("Processing tile "+str(currentTile)+" of "+str(maxTile))
    if progress >= 100:
      if self.ui.loadCheckBox.isChecked() and os.path.exists(self.ui.outputLineEdit.text()):
	layer = QgsRasterLayer(self.ui.outputLineEdit.text())
	QgsMapLayerRegistry.instance().addMapLayer(layer, True)
	layerSet = []
	mapCanvasLayer = QgsMapCanvasLayer(layer, True)
	layerSet.append(mapCanvasLayer)
	self.iface.mapCanvas().setExtent(layer.extent())
	self.iface.mapCanvas().setLayerSet(layerSet)
      self.setStartButton()
     
  def setStartButton(self):
    self.ui.tabWidget.setTabEnabled(0, True)
    self.ui.tabWidget.setTabEnabled(1, True)
    self.ui.tabWidget.setTabEnabled(2, True)
    self.ui.tabWidget.setTabEnabled(3, True)
    self.ui.startButton.show()
    self.ui.stopButton.hide()
    
  def setStopButton(self):
    self.ui.tabWidget.setTabEnabled(0, False)
    self.ui.tabWidget.setTabEnabled(1, False)
    self.ui.tabWidget.setTabEnabled(2, False)
    self.ui.tabWidget.setTabEnabled(3, False)
    self.ui.startButton.hide()
    self.ui.stopButton.show()
     
  def stop(self):
    self.setStartButton()
    self.extractor.stop()
      
  def start(self):
    self.setStopButton()
    self.clearLog()
    self.extractor = ImageExtractor()
    QObject.connect(self.extractor, SIGNAL("progressed(double, int, int)"), self.progress)
    QObject.connect(self.extractor, SIGNAL("logged(QString)"), self.appendLog)
    self.extractor.outputPath = self.ui.outputLineEdit.text()
    if self.ui.maxRadioButton.isChecked():
      devider = -1
    else:
      devider = self.ui.manualSpinBox.value()
    self.extractor.layerName = self.ui.sourceComboBox.currentText()
    self.extractor.redownloadLevel = self.ui.redownloadSlider.value()
    self.extractor.skipLevel = self.ui.skipSlider.value()
    self.extractor.cutOverhead = self.ui.overHeadCheckBox.isChecked()
    if self.ui.proxyCheckBox.isChecked() and self.setupProxy() != None:
      self.extractor.setProxy(self.proxy)
    self.extractor.extractArea(Extent(float(self.ui.minX.text()), float(self.ui.minY.text()), float(self.ui.maxX.text()), float(self.ui.maxY.text())), devider)
    
  def setupProxy(self):
    settings = QSettings()
    settings.beginGroup("proxy")
    if settings.value("/proxyEnabled").toBool():
      proxyType = settings.value( "/proxyType", QVariant(0)).toString()
      if proxyType in ["1","Socks5Proxy"]:
	self.proxy.setType(QNetworkProxy.Socks5Proxy)
      elif proxyType in ["2","NoProxy"]:
	self.proxy.setType(QNetworkProxy.NoProxy)
      elif proxyType in ["3","HttpProxy"]:
	self.proxy.setType(QNetworkProxy.HttpProxy)
      elif proxyType in ["4","HttpCachingProxy"] and QT_VERSION >= 0X040400:
	self.proxy.setType(QNetworkProxy.HttpCachingProxy)
      elif proxyType in ["5","FtpCachingProxy"] and QT_VERSION >= 0X040400:
	self.proxy.setType(QNetworkProxy.FtpCachingProxy)
      else:
	self.proxy.setType(QNetworkProxy.DefaultProxy)
      self.proxy.setHostName(settings.value("/proxyHost").toString())
      self.proxy.setPort(settings.value("/proxyPort").toUInt()[0])
      self.proxy.setUser(settings.value("/proxyUser").toString())
      self.proxy.setPassword(settings.value("/proxyPassword").toString())
      return self.proxy
    else:
      return None
    settings.endGroup()