# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from qgis.analysis import *
from ui_linecorrector import Ui_LineCorrector

class LineCorrectorWindow(QDialog):

  def getGui(self):
    return self

  def __init__(self, iface, standAlone = True):
    QMainWindow.__init__( self )
    self.ui = Ui_LineCorrector()
    self.iface = iface
    self.ui.setupUi( self )
    
    self.standAlone = standAlone
    if not self.standAlone:
      self.ui.tabWidget.removeTab(self.ui.tabWidget.count()-1)
    
    QObject.connect(self.ui.inputButton, SIGNAL("clicked()"), self.selectInput)
    QObject.connect(self.ui.outputButton, SIGNAL("clicked()"), self.selectOuput)
    QObject.connect(self.ui.maskButton, SIGNAL("clicked()"), self.selectMask)
    
    QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Ok), SIGNAL("clicked()"), self.start)
    QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Cancel), SIGNAL("clicked()"), self.stop)
    #QObject.connect(self.ui.helpButton, SIGNAL("clicked()"), self.help)
    self.toggleOutputTasks()
    self.ui.tabWidget.setCurrentIndex(0)
    self.ui.buttonBox.button(QDialogButtonBox.Ok).setText("Run")
    
    self.tempBands = []
    
  def toggleOutputTasks(self, state = 0):
    self.ui.outputLineEdit.setVisible(False)
    self.ui.outputButton.setVisible(False)
    self.ui.maskLineEdit.setVisible(False)
    self.ui.maskButton.setVisible(False)
    
  def selectInput(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("lineCorrector/lastImageDir").toString()
    fileName = QFileDialog.getOpenFileName(self, "Select Input Image", myLastDir)
    if fileName != "":
      self.ui.inputLineEdit.setText(fileName)
      mySettings.setValue("lineCorrector/lastImageDir", QFileInfo( fileName ).absolutePath())
      
  def selectOuput(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("lineCorrector/outputDir").toString()
    fileName = QFileDialog.getSaveFileName(self, "Save Output Image", myLastDir)
    if fileName != "":
      self.ui.outputLineEdit.setText(fileName)
      mySettings.setValue("lineCorrector/outputDir", QFileInfo( fileName ).absolutePath())
      
  def selectMask(self):
    mySettings = QSettings()
    myLastDir = mySettings.value("lineCorrector/outputMaskDir").toString()
    fileName = QFileDialog.getSaveFileName(self, "Save Output Mask", myLastDir)
    if fileName != "":
      self.ui.maskLineEdit.setText(fileName)
      mySettings.setValue("lineCorrector/outputMaskDir", QFileInfo( fileName ).absolutePath())
      
  def start(self):
    myState = self.ui.buttonBox.button(QDialogButtonBox.Ok).text()
    if myState == "Stop":
      self.stop()
      return
    else:
      #sanity check...
      if self.ui.inputLineEdit.text().isEmpty(): 
        QMessageBox.warning(self, qApp.tr("Line Corrector"), qApp.tr("No input file(s) specified.\n"
                                 "Please correct this before pressing 'run'!"),
                                QMessageBox.Ok );
        return
      self.setStopButton()

    self.corrector = QgsLineCorrector(self.ui.inputLineEdit.text(), self.ui.outputLineEdit.text(), self.ui.maskLineEdit.text())
    
    phase1 = QgsLineCorrector.None
    index1 = self.ui.phase1ComboBox.currentIndex()
    if index1 == 1:
      phase1 = QgsLineCorrector.ColumnBased
    elif index1 == 2:
      phase1 = QgsLineCorrector.LineBased
      
    phase2 = QgsLineCorrector.None
    index2 = self.ui.phase2ComboBox.currentIndex()
    if index2 == 1:
      phase2 = QgsLineCorrector.ColumnBased
    elif index2 == 2:
      phase2 = QgsLineCorrector.LineBased

    self.corrector.setPhases(phase1, phase2, self.ui.recalculateCheckBox.isChecked())
    self.corrector.setCreateMask(self.ui.maskCheckBox.isChecked())
    QObject.connect(self.corrector, SIGNAL("progressed(double, double, QString)"), self.progress)
    QObject.connect(self.corrector, SIGNAL("logged(QString)"), self.log)
    QObject.connect(self.corrector, SIGNAL("badLinesFound(QList<int>)"), self.badLines)
    self.ui.progressBar.setValue(0)
    self.ui.textEdit.clear()
    self.ui.tabWidget.setCurrentIndex(1)
    self.corrector.start()
  
  def progress(self, value1, value2, string):
    self.ui.totalProgressBar.setValue(value1)
    self.ui.partProgressBar.setValue(value2)
    self.ui.partLabel.setText(string)
    if value1 == 100:
      self.setStartButton()
      self.loadResult()
    
  def log(self, value):
    self.ui.log.append(value)
    
  def badLines(self, values):
    for value in values:
      self.ui.listWidget.addItem(str(value))
  
  def stop(self):
    if self.ui.buttonBox.button(QDialogButtonBox.Ok).text() == "Stop":
      self.corrector.stop()
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
