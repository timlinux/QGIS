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
from qgis.analysis import *
from ui_columncorrector import Ui_ColumnCorrector
import time
import osgeo.gdal as gdal
from osgeo.gdalconst import *

class ColumnCorrectorWindow(QMainWindow):

  def getGui(self):
    return self.ui.centralwidget

  def __init__(self, iface, standAlone = True):
    QMainWindow.__init__( self )
    self.ui = Ui_ColumnCorrector()
    self.iface = iface
    self.ui.setupUi( self )
    
    self.standAlone = standAlone
    if not self.standAlone:
      self.ui.tabWidget.removeTab(self.ui.tabWidget.count()-1)

    self.connect(self.ui.maskCheckBox, SIGNAL("stateChanged(int)"), self.toggleOutputTasks)
    self.connect(self.ui.exportCheckBox, SIGNAL("stateChanged(int)"), self.toggleOutputTasks)
    self.connect(self.ui.correctCheckBox, SIGNAL("stateChanged(int)"), self.toggleOutputTasks)
    self.connect(self.ui.loadRadioButton, SIGNAL("clicked()"), self.toggleBadColumns)
    self.connect(self.ui.computeRadioButton, SIGNAL("clicked()"), self.toggleBadColumns)
    
    self.connect(self.ui.rasterButton, SIGNAL("pressed()"), self.openRaster)
    self.connect(self.ui.loadButton, SIGNAL("pressed()"), self.openBadFile)
    self.connect(self.ui.exportButton, SIGNAL("pressed()"), self.saveBadFile)
    self.connect(self.ui.correctButton, SIGNAL("pressed()"), self.saveCorrectFile)
    self.connect(self.ui.maskButton, SIGNAL("pressed()"), self.saveMaskFile)
    
    self.connect(self.ui.nextButton, SIGNAL("pressed()"), self.toggleButtonNext)
    self.connect(self.ui.backButton, SIGNAL("pressed()"), self.toggleButtonBack)
    self.connect(self.ui.startButton, SIGNAL("pressed()"), self.startProcess)
    self.connect(self.ui.stopButton, SIGNAL("pressed()"), self.stopProcess)
    self.connect(self.ui.tabWidget, SIGNAL("currentChanged(int)"), self.toggleTab)
    self.toggleOutputTasks()
    self.toggleBadColumns()
    self.loadRaster(self.getDefaultRaster())
    
    self.settings = QSettings("QuantumGIS", "SumbandilaSat Georeferencer")
    self.loadSettings()
    
    self.ui.tabWidget.setCurrentIndex(0)
    self.changeButtons(0)
    #self.loadRaster(QString("/home/cstallmann/I03B1_P03_S02_C02_F03_MSSK14K_0.tif"))
    
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
    
  def showOpenFileDialog(self, title = "Open", defaultDir = "", fileFilter = ""):
    return QFileDialog.getOpenFileName( self, title, defaultDir, fileFilter)
    
  def showSaveFileDialog(self, title = "Save", defaultDir = "", fileFilter = ""):
    return QFileDialog.getSaveFileName( self, title, defaultDir, fileFilter)
    
  def openRaster(self):
    path = self.showOpenFileDialog("Open Raster Image", "", "")
    self.loadRaster(path)
    
  def openBadFile(self):
    path = self.showOpenFileDialog("Open Bad Columns", "", "Bad Column Files (*.bad)")
    self.ui.loadLineEdit.setText(path)
    
  def saveBadFile(self):
    path = self.showSaveFileDialog("Save Bad Columns", "", "Bad Column Files (*.bad)")
    self.ui.exportLineEdit.setText(path)
    
  def saveCorrectFile(self):
    path = self.showSaveFileDialog("Save Corrected Raster Image", "", "")
    self.ui.correctLineEdit.setText(path)
    
  def saveMaskFile(self):
    path = self.showSaveFileDialog("Save Check Mask", "", "")
    self.ui.maskLineEdit.setText(path)
    
  def getDefaultRaster(self):
    if self.iface.activeLayer() != None:
      return self.iface.activeLayer().source()
    return ""
    
  def loadRaster(self, path):
    if path != "":
      self.ui.rasterLineEdit.setText(path)
      self.ui.exportLineEdit.setText(path+".bad")
      self.ui.loadLineEdit.setText(path+".bad")
      self.ui.correctLineEdit.setText(path+".corrected")
      self.ui.maskLineEdit.setText(path+".check")
      dataset = gdal.Open(path.toAscii().data(), GA_ReadOnly)
      columns = dataset.RasterXSize
      self.ui.startCol.setValue(0)
      self.ui.stopCol.setValue(columns)
      #dataset = None
    
  def setStartButton(self):
    if self.standAlone:
      self.isRunning = False
      self.ui.startButton.show()
      self.ui.stopButton.hide()
      self.ui.tabWidget.setTabEnabled(0, True)
      self.ui.tabWidget.setTabEnabled(1, True)
      self.ui.tabWidget.setTabEnabled(2, True)
    
  def setStopButton(self):
    if self.standAlone:
      self.isRunning = True
      self.ui.startButton.hide()
      self.ui.stopButton.show()
      self.ui.tabWidget.setTabEnabled(0, False)
      self.ui.tabWidget.setTabEnabled(1, False)
      self.ui.tabWidget.setTabEnabled(2, False)
    
  def toggleBadColumns(self):
    if self.ui.loadRadioButton.isChecked():
      self.ui.loadWidget.show()
    else:
      self.ui.loadWidget.hide()
    
  def changeButtons(self, index):
    self.ui.nextButton.hide()
    self.ui.backButton.hide()
    self.ui.startButton.hide()
    self.ui.stopButton.hide()
    if index == 0:
      self.ui.nextButton.show()
    elif index == self.ui.tabWidget.count()-1:
      self.ui.backButton.show()
      self.setStartButton()
    else:
      self.ui.backButton.show()
      self.ui.nextButton.show()
    
  def toggleButtonNext(self):
    index = self.ui.tabWidget.currentIndex()
    index += 1
    self.ui.tabWidget.setCurrentIndex(index)
    self.changeButtons(index)
    
  def toggleButtonBack(self):
    index = self.ui.tabWidget.currentIndex()
    if index != 0:
      index -= 1
      self.ui.tabWidget.setCurrentIndex(index)
      self.changeButtons(index)
    
  def stopProcess(self):
    self.disconnect(self.processor, SIGNAL("updated(int,QString, int)"), self.update)
    self.disconnect(self.processor, SIGNAL("updatedLog(QString)"), self.updateLog)
    self.disconnect(self.processor, SIGNAL("updatedColumns(QString)"), self.updateColumns)
    self.processor.stop()
    while self.processor.isRunning():
      pass
    self.processor.releaseMemory()
    del self.processor
    self.processor = None
    self.setStartButton()
    
  def toggleTab(self, index):
    self.changeButtons(index)
    
  def toggleOutputTasks(self, state = 0):
    if self.ui.exportCheckBox.isChecked():
      self.ui.exportWidget.show()
    else:
      self.ui.exportWidget.hide()
    if self.ui.maskCheckBox.isChecked():
      self.ui.maskWidget.show()
    else:
      self.ui.maskWidget.hide()
    if self.ui.correctCheckBox.isChecked():
      self.ui.correctWidget.show()
      self.ui.maskCheckBox.setEnabled(True)
    else:
      self.ui.correctWidget.hide()
      self.ui.maskCheckBox.setChecked(False)
      self.ui.maskCheckBox.setEnabled(False)
      self.ui.maskWidget.hide()
    
  def loadSettings(self):
    pass
    
  def saveSettings(self):
    pass
    
  def startProcess(self):
    self.ui.listWidget.clear()
    self.ui.log.clear()
    self.processor = QgsImageProcessor()
    self.connect(self.processor, SIGNAL("updated(int,QString, int)"), self.update)
    self.connect(self.processor, SIGNAL("updatedLog(QString)"), self.updateLog)
    self.connect(self.processor, SIGNAL("updatedColumns(QString)"), self.updateColumns)
    self.processor.setInputPath(self.ui.rasterLineEdit.text())
    
    if self.ui.loadRadioButton.isChecked():
      self.processor.setInFilePath(self.ui.loadLineEdit.text())
    else:
      self.processor.setInFilePath("")
    
    if self.ui.exportCheckBox.isChecked():
      self.processor.setOutFilePath(self.ui.exportLineEdit.text())
    else:
      self.processor.setOutFilePath("")
      
    if self.ui.correctCheckBox.isChecked():
      self.processor.setOutputPath(self.ui.correctLineEdit.text())
    else:
      self.processor.setOutputPath("")
      
    if self.ui.maskCheckBox.isChecked():
      self.processor.setCheckPath(self.ui.maskLineEdit.text())
    else:
      self.processor.setCheckPath("")
      
    self.processor.setThresholds(self.ui.gainThreshold.value(), self.ui.biasThreshold.value())
    self.processor.start(-1, -1)
    
    self.setStopButton()
  
  def update(self, progress, action, totalProgress):
    if action == "Finished":
      self.setStartButton()
      self.ui.taskProgressBar.setValue(100)
      self.ui.totalProgressBar.setValue(100)
      self.disconnect(self.processor, SIGNAL("updated(int,QString, int)"), self.update)
      self.disconnect(self.processor, SIGNAL("updatedLog(QString)"), self.updateLog)
      self.disconnect(self.processor, SIGNAL("updatedColumns(QString)"), self.updateColumns)
      self.processor.releaseMemory()
      self.processor = None
    else:
      self.ui.taskLabel.setText(action)
      self.ui.taskProgressBar.setValue(progress)
      self.ui.totalProgressBar.setValue(totalProgress)
    
  def updateLog(self, string):
    self.ui.log.append(string)
      
  def updateColumns(self, columns):
    self.ui.listWidget.addItem(columns)