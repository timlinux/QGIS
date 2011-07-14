# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from ui_bandaligner import Ui_BandAligner
from qgis.analysis import *

import gdal

class BandAlignerWindow(QMainWindow):

  def getGui(self):
    return self.ui.centralwidget

  def __init__(self, iface, standAlone = True):
    QMainWindow.__init__( self )
    self.ui = Ui_BandAligner()
    self.iface = iface
    self.ui.setupUi( self )
    
    self.standAlone = standAlone
    if not self.standAlone:
      self.ui.tabWidget.removeTab(self.ui.tabWidget.count()-1)
    
    QObject.connect(self.ui.singleRadioButton, SIGNAL("clicked()"), self.selectFiles)
    QObject.connect(self.ui.multiRadioButton, SIGNAL("clicked()"), self.selectFiles)
    QObject.connect(self.ui.checkBox, SIGNAL("stateChanged(int)"), self.toggleDisparity)
    
    QObject.connect(self.ui.bandLineEdit, SIGNAL("textChanged(const QString &)"), self.updateReferenceBand)
    QObject.connect(self.ui.bandButton, SIGNAL("clicked()"), self.selectInput)
    QObject.connect(self.ui.addButton, SIGNAL("clicked()"), self.addBands)
    QObject.connect(self.ui.outputButton, SIGNAL("clicked()"), self.selectOuput)
    
    QObject.connect(self.ui.backButton, SIGNAL("clicked()"), self.goBack)
    QObject.connect(self.ui.nextButton, SIGNAL("clicked()"), self.goNext)
    QObject.connect(self.ui.startButton, SIGNAL("clicked()"), self.start)
    QObject.connect(self.ui.stopButton, SIGNAL("clicked()"), self.stop)
    #QObject.connect(self.ui.helpButton, SIGNAL("clicked()"), self.help)
    QObject.connect(self.ui.tabWidget, SIGNAL("currentChanged(int)"), self.changeTab)
    
    self.settings = QSettings("QuantumGIS", "SumbandilaSat Band Aligner")
    
    self.selectFiles()
    self.toggleDisparity()
    self.changeTab()
    self.ui.tabWidget.setCurrentIndex(0)
    
    self.tempBands = []
    self.thread = None
    
  def toggleDisparity(self, value = 0):
    if self.ui.checkBox.isChecked():
      self.ui.disparityWidget.show()
    else:
      self.ui.disparityWidget.hide()
    
  def selectFiles(self):
    self.ui.referenceComboBox.clear()
    self.ui.bandLineEdit.clear()
    self.ui.tableWidget.clearContents()
    if self.ui.singleRadioButton.isChecked():
      self.ui.addButton.hide()
      self.ui.tableWidget.hide()
    else:
      self.ui.addButton.show()
      self.ui.tableWidget.show()
    
  def selectOuput(self):
    fileName = QFileDialog.getSaveFileName(self, "Save Output Image", self.settings.value("LastPath", QVariant("")).toString())
    if fileName != "":
      self.settings.setValue("LastPath", fileName)
      self.ui.outputLineEdit.setText(fileName)
      if self.ui.xLineEdit.text() != "":
	self.ui.xLineEdit.setText(fileName+".xmap")
      if self.ui.yLineEdit.text() != "":
	self.ui.yLineEdit.setText(fileName+".ymap")
    
  def getAllInputBands(self, withId = False, entirePath = True):
    result = []
    if self.ui.singleRadioButton.isChecked():
      result.append(self.ui.bandLineEdit.text())
    else:
      for i in range(self.ui.tableWidget.rowCount()):
	name = ""
	if entirePath:
	  name = self.ui.tableWidget.item(i, 0).text()
	else:
	  name = self.ui.tableWidget.item(i, 0).text()
	  index = name.lastIndexOf(QDir.separator())
	  name = name.right(name.length()-index-1)
	if withId:
	  result.append(str(i+1)+". "+name)
	else:
	  result.append(name)
    return result
    
  def updateReferenceBand(self, band = ""):
    index = self.ui.referenceComboBox.currentIndex()
    names = self.getAllInputBands(True, False)
    self.ui.referenceComboBox.clear()
    if self.ui.singleRadioButton.isChecked():
      dataset = gdal.Open(self.ui.bandLineEdit.text().toAscii().data(), gdal.GA_ReadOnly)
      if dataset != None:
	count = dataset.RasterCount
	for i in range(count):
	  self.ui.referenceComboBox.addItem(str(i+1)+". Band "+str(i+1))
    else:
      for name in names:
	self.ui.referenceComboBox.addItem(name)
    if self.ui.referenceComboBox.count() > index:
      self.ui.referenceComboBox.setCurrentIndex(index)
    
  def addBands(self):
    currentRows = self.ui.tableWidget.rowCount()
    self.ui.tableWidget.setRowCount(currentRows+len(self.tempBands))
    if len(self.tempBands) == 1:
      itemPath = QTableWidgetItem(self.ui.bandLineEdit.text())
      self.ui.tableWidget.setItem(currentRows, 0, itemPath)
    else:
      counter = 0
      for band in self.tempBands:
	itemPath = QTableWidgetItem(self.tempBands[counter])
	self.ui.tableWidget.setItem(currentRows+counter, 0, itemPath)
	counter += 1
    self.tempBands = []
    self.ui.bandLineEdit.setText("")
    self.updateReferenceBand()
    
  def selectInput(self):
    self.tempBands = []
    self.ui.bandLineEdit.setText("")
    fileNames = QFileDialog.getOpenFileNames(self, "Select Input Bands", self.settings.value("LastPath", QVariant("")).toString())
    if len(fileNames) > 1:
      self.settings.setValue("LastPath", fileNames[0])
      self.ui.bandLineEdit.setText("<Multiple Files>")
    elif len(fileNames) == 1:
      self.settings.setValue("LastPath", fileNames[0])
      self.ui.bandLineEdit.setText(fileNames[0])
    for f in fileNames:
      self.tempBands.append(f)
    self.updateReferenceBand()
    
  def changeTab(self, i = 0):
    index = self.ui.tabWidget.currentIndex()
    self.ui.startButton.hide()
    self.ui.stopButton.hide()
    self.ui.backButton.hide()
    self.ui.nextButton.hide()
    if index == 0:
      self.ui.nextButton.show()
    elif index == self.ui.tabWidget.count()-1:
      self.ui.backButton.show()
      self.setStartButton()
    else:
      self.ui.backButton.show()
      self.ui.nextButton.show()
      
  def goBack(self):
    if self.ui.tabWidget.currentIndex() > 0:
      self.ui.tabWidget.setCurrentIndex(self.ui.tabWidget.currentIndex()-1)
      
  def goNext(self):
    if self.ui.tabWidget.currentIndex() < self.ui.tabWidget.count()-1:
      self.ui.tabWidget.setCurrentIndex(self.ui.tabWidget.currentIndex()+1)
      
  def progress(self, value):
    self.ui.progressBar.setValue(value)
    if value >= 100:
      self.setStartButton()
    
  def log(self, message):
    self.ui.textEdit.append(message)
      
  def start(self):
    self.ui.progressBar.setValue(0)
    self.ui.textEdit.clear()
    inputPaths = self.getAllInputBands()
    outputPath = self.ui.outputLineEdit.text()
    xPath = ""
    yPath = ""
    if self.ui.checkBox.isChecked():
      xPath = self.ui.xLineEdit.text()
      yPath = self.ui.yLineEdit.text()
    self.thread = QgsBandAligner(inputPaths, outputPath, xPath, yPath, self.ui.spinBox.value(), self.ui.referenceComboBox.currentIndex()+1)
    QObject.connect(self.thread, SIGNAL("progressed(double)"), self.progress)
    QObject.connect(self.thread, SIGNAL("logged(QString)"), self.log)
    self.thread.start()
    self.setStopButton()
  
  def stop(self):
    self.thread.stop()
  
  def help(self):
    pass  
  
  def setStopButton(self):
    if self.standAlone:
      self.ui.startButton.hide()
      self.ui.stopButton.show()
    
  def setStartButton(self):
    if self.standAlone:
      self.ui.startButton.show()
      self.ui.stopButton.hide()
