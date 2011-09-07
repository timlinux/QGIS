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

class ColumnCorrectorWindow(QDialog):

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
    
    self.connect(self.ui.rasterButton, SIGNAL("pressed()"), self.openRaster)
    self.connect(self.ui.loadButton, SIGNAL("pressed()"), self.openBadFile)
    self.connect(self.ui.exportButton, SIGNAL("pressed()"), self.saveBadFile)
    self.connect(self.ui.correctButton, SIGNAL("pressed()"), self.saveCorrectFile)
    self.connect(self.ui.maskButton, SIGNAL("pressed()"), self.saveMaskFile)
    QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Ok), SIGNAL("clicked()"), self.startProcess)
    QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Cancel), SIGNAL("clicked()"), self.stopProcess)

    self.ui.loadRadioButton.setChecked(True) # we actually want if false so...
    self.ui.computeRadioButton.toggle() #trigger so form updates to hide stuff
    self.ui.exportCheckBox.setChecked(True) # we actually want if false so...
    self.ui.exportCheckBox.toggle() #trigger so form updates to hide stuff
    self.ui.correctCheckBox.setChecked(False) # we actually want if true so...
    self.ui.correctCheckBox.toggle() #trigger so form updates to show stuff
    self.ui.maskCheckBox.setChecked(True) # we actually want if false so...
    self.ui.maskCheckBox.toggle() #trigger so form updates to hide stuff
    
    self.loadRaster(self.getDefaultRaster())
    self.settings = QSettings("QuantumGIS", "SumbandilaSat Georeferencer")
    self.loadSettings()
    self.ui.tabWidget.setCurrentIndex(0)
    
  #Displays a message to the user
  def showMessage(self, string, type = "info"):
    msgbox = QMessageBox(self.ui.centralwidget)
    msgbox.setText(string)
    msgbox.setWindowTitle("Column Corrector")
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
    mySettings = QSettings()
    myLastDir = mySettings.value("columnCorrector/lastInputDir").toString()
    path = self.showOpenFileDialog("Open Raster Image", myLastDir, "")
    if path != "":
      self.loadRaster(path)
      mySettings.setValue("columnCorrector/lastInputDir", QFileInfo( path ).absolutePath())
    
  def openBadFile(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("columnCorrector/lastBadColumnsDir").toString()
    path = self.showOpenFileDialog("Open Bad Columns", myLastDir, "Bad Column Files (*.bad)")
    if path != "":
      self.ui.loadLineEdit.setText(path)
      mySettings.setValue("columnCorrector/lastBadColumnsDir", QFileInfo( path ).absolutePath())
    
  def saveBadFile(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("columnCorrector/lastBadColumnsDir").toString()
    path = self.showSaveFileDialog("Save Bad Columns", myLastDir, "Bad Column Files (*.bad)")
    if path != "":
      self.ui.exportLineEdit.setText(path)
      mySettings.setValue("columnCorrector/lastBadColumnsDir", QFileInfo( path ).absolutePath())
    
  def saveCorrectFile(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("columnCorrector/lastCorrectFileDir").toString()
    path = self.showSaveFileDialog("Save Corrected Raster Image", myLastDir, "")
    if path != "":
      self.ui.correctLineEdit.setText(path)
      mySettings.setValue("columnCorrector/lastCorrectFileDir", QFileInfo( path ).absolutePath())
    
  def saveMaskFile(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("columnMaskor/lastMaskFileDir").toString()
    path = self.showSaveFileDialog("Save Check Mask", myLastDir, "")
    if path != "":
      self.ui.maskLineEdit.setText(path)
      mySettings.setValue("columnMaskor/lastMaskFileDir", QFileInfo( path ).absolutePath())
    
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
    
    
  def loadSettings(self):
    pass
    
  def saveSettings(self):
    pass
    
  def startProcess(self):
    myState = self.ui.buttonBox.button(QDialogButtonBox.Ok).text()
    if myState == "Stop":
      self.thread.stop()
      return

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
    # name of the bad columns text file
    if self.ui.exportCheckBox.isChecked():
      self.processor.setOutFilePath(self.ui.exportLineEdit.text())
    else:
      self.processor.setOutFilePath("")
    #name of the outpu raster 
    if self.ui.correctCheckBox.isChecked():
      self.processor.setOutputPath(self.ui.correctLineEdit.text())
    else:
      self.processor.setOutputPath("")
    # name of the mask output file  
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
      self.loadResult()
    else:
      self.ui.taskLabel.setText(action)
      self.ui.taskProgressBar.setValue(progress)
      self.ui.totalProgressBar.setValue(totalProgress)
    
  def updateLog(self, string):
    self.ui.log.append(string)
      
  def updateColumns(self, columns):
    self.ui.listWidget.addItem(columns)


  def stopProcess(self):
    if self.ui.buttonBox.button(QDialogButtonBox.Ok).text() == "Stop":
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

    else:
      # close the dialog
      self.close()
  
  def help(self):
    pass  
  
  def setStopButton(self):
    if self.standAlone:
      self.isRunning = True
      self.ui.buttonBox.button(QDialogButtonBox.Ok).hide()
      #stop label used programmatically to see if we are actively running or not
      self.ui.buttonBox.button(QDialogButtonBox.Ok).setText("Stop")
      self.ui.buttonBox.button(QDialogButtonBox.Cancel).setText("Cancel")
      self.ui.tabWidget.setTabEnabled(0, False)
      self.ui.tabWidget.setTabEnabled(1, False)
      self.ui.tabWidget.setCurrentIndex(1)
    
  def setStartButton(self):
    if self.standAlone:
      self.isRunning = False
      self.ui.buttonBox.button(QDialogButtonBox.Ok).show()
      self.ui.buttonBox.button(QDialogButtonBox.Ok).setText("Run")
      self.ui.buttonBox.button(QDialogButtonBox.Cancel).setText("Close")
      self.ui.tabWidget.setTabEnabled(0, True)
      self.ui.tabWidget.setTabEnabled(1, True)

  def loadResult(self):
    fileName = None
    if self.ui.correctCheckBox.isChecked():
      # corrected the original image so load it now
      fileName = self.ui.rasterLineEdit.text()
    else:
      # corrected image was saved to a new file so load that...
      fileName = self.ui.correctLineEdit.text()
    #now load it in the gui
    fileInfo = QFileInfo(fileName)
    self.iface.addRasterLayer(fileInfo.filePath())

