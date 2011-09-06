# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from ui_bandaligner import Ui_BandAligner
from qgis.analysis import *

import gdal

class BandAlignerWindow(QDialog):

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
    QObject.connect(self.ui.outputButton, SIGNAL("clicked()"), self.selectOutput)
    QObject.connect(self.ui.xButton, SIGNAL("clicked()"), self.selectXOutput)
    QObject.connect(self.ui.yButton, SIGNAL("clicked()"), self.selectYOutput)
    
    QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Ok), SIGNAL("clicked()"), self.start)
    QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Cancel), SIGNAL("clicked()"), self.stop)
    #QObject.connect(self.ui.helpButton, SIGNAL("clicked()"), self.help)
    
    self.settings = QSettings("QuantumGIS", "SumbandilaSat Band Aligner")
    
    self.selectFiles()
    self.toggleDisparity()
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

  def selectXOutput(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("bandAligner/lastXOutputDir").toString()
    fileName = QFileDialog.getSaveFileName(self, "Save Output X Image", myLastDir )
    if fileName != "":
      self.ui.xLineEdit.setText(fileName)
      mySettings.setValue("bandAligner/lastXOutputDir", QFileInfo( fileName ).absolutePath())
    
  def selectYOutput(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("bandAligner/lastYOutputDir").toString()
    fileName = QFileDialog.getSaveFileName(self, "Save Output Y Image", myLastDir )
    if fileName != "":
      self.ui.yLineEdit.setText(fileName)
      mySettings.setValue("bandAligner/lastYOutputDir", QFileInfo( fileName ).absolutePath())
    
    
  def selectOutput(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("bandAligner/lastOutputDir").toString()
    fileName = QFileDialog.getSaveFileName(self, "Save Output Image", myLastDir )
    if fileName != "":
      self.ui.outputLineEdit.setText(fileName)
      myInfo = QFileInfo( fileName )
      mySettings.setValue("bandAligner/lastOutputDir", myInfo.absolutePath())
      if self.ui.xLineEdit.text().isEmpty():
        myXName = fileName.replace( myInfo.completeSuffix(),"" ) + "xmap.tif"   
        self.ui.xLineEdit.setText( myXName )
      if self.ui.yLineEdit.text().isEmpty():
        myYName = fileName.replace( myInfo.completeSuffix(),"" ) + "ymap.tif"   
        self.ui.yLineEdit.setText( myYName )
    
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
    mySettings = QSettings()
    myLastDir = mySettings.value("bandAligner/lastInputDir").toString()
    fileNames = QFileDialog.getOpenFileNames(self, "Select Input Bands", myLastDir )
    if len(fileNames) > 1:
      self.ui.bandLineEdit.setText("<Multiple Files>")
    elif len(fileNames) == 1:
      self.ui.bandLineEdit.setText(fileNames[0])
    if len(fileNames) > 0:
      mySettings.setValue("bandAligner/lastInputDir", QFileInfo( fileNames[0] ).absolutePath())
    for f in fileNames:
      self.tempBands.append(f)
    self.updateReferenceBand()

  def setStartButton(self):
    if self.standAlone:
      self.ui.buttonBox.button(QDialogButtonBox.Ok).show()
      self.ui.buttonBox.button(QDialogButtonBox.Cancel).setText("Close")
      self.ui.tabWidget.setTabEnabled(0, True)
      self.ui.tabWidget.setTabEnabled(1, True)
    
  def setStopButton(self):
    if self.standAlone:
      self.ui.buttonBox.button(QDialogButtonBox.Ok).hide()
      self.ui.buttonBox.button(QDialogButtonBox.Cancel).setText("Cancel")
      self.ui.tabWidget.setTabEnabled(0, False)
      self.ui.tabWidget.setTabEnabled(1, False)
    
  def progress(self, value):
    self.ui.progressBar.setValue(value)
    if value >= 100:
      self.setStartButton()
      self.loadResult()
    
  def log(self, message):
    self.ui.textEdit.append(message)
      
  def start(self):
    myState = self.ui.buttonBox.button(QDialogButtonBox.Ok).text()
    if myState == "Stop":
      self.thread.stop()
      return

    inputPaths = self.getAllInputBands()
    outputPath = self.ui.outputLineEdit.text()
    #sanity check...
    if len(inputPaths) == 0:
      QMessageBox.warning(self, qApp.tr("Band Aligner"), qApp.tr("No input file(s) specified.\n"
                                 "Please correct this before pressing 'run'!"),
                                QMessageBox.Ok );
      return
    if outputPath.isEmpty():
      QMessageBox.warning(self, qApp.tr("Band Aligner"), qApp.tr("No output file specified.\n"
                                 "Please correct this before pressing 'run'!"),
                                QMessageBox.Ok );
      return
    self.ui.progressBar.setValue(0)
    self.ui.textEdit.clear()
    self.ui.tabWidget.setCurrentIndex(1)
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

    if self.ui.buttonBox.button(QDialogButtonBox.Ok).text() == "Stop":
      self.thread.stop()
      self.setStartButton()
    else:
      # close the dialog
      self.close()
  
  def help(self):
    pass  
  
  def setStopButton(self):
    if self.standAlone:
      self.ui.buttonBox.button(QDialogButtonBox.Ok).hide()
      #stop label used programmatically to see if we are actively running or not
      self.ui.buttonBox.button(QDialogButtonBox.Ok).setText("Stop")
      self.ui.buttonBox.button(QDialogButtonBox.Cancel).setText("Cancel")
      self.ui.tabWidget.setTabEnabled(0, False)
      self.ui.tabWidget.setTabEnabled(1, False)
      self.ui.tabWidget.setCurrentIndex(1)
    
  def setStartButton(self):
    if self.standAlone:
      self.ui.buttonBox.button(QDialogButtonBox.Ok).show()
      self.ui.buttonBox.button(QDialogButtonBox.Ok).setText("Run")
      self.ui.buttonBox.button(QDialogButtonBox.Cancel).setText("Close")
      self.ui.tabWidget.setTabEnabled(0, True)
      self.ui.tabWidget.setTabEnabled(1, True)

  def loadResult(self):
    fileName = self.ui.outputLineEdit.text()
    fileInfo = QFileInfo(fileName)
    self.iface.addRasterLayer(fileInfo.filePath())

