# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from qgis.analysis import *
from ui_linecorrector import Ui_LineCorrector

class LineCorrectorWindow(QMainWindow):

  def getGui(self):
    return self.ui.centralwidget

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
    self.connect(self.ui.maskCheckBox, SIGNAL("stateChanged(int)"), self.toggleOutputTasks)
    self.connect(self.ui.outputCheckBox, SIGNAL("stateChanged(int)"), self.toggleOutputTasks)
    
    QObject.connect(self.ui.backButton, SIGNAL("clicked()"), self.goBack)
    QObject.connect(self.ui.nextButton, SIGNAL("clicked()"), self.goNext)
    QObject.connect(self.ui.startButton, SIGNAL("clicked()"), self.start)
    QObject.connect(self.ui.stopButton, SIGNAL("clicked()"), self.stop)
    #QObject.connect(self.ui.helpButton, SIGNAL("clicked()"), self.help)
    QObject.connect(self.ui.tabWidget, SIGNAL("currentChanged(int)"), self.changeTab)
    self.changeTab()
    self.toggleOutputTasks()
    self.ui.tabWidget.setCurrentIndex(0)
    
    self.tempBands = []
    
  def toggleOutputTasks(self, state = 0):
    if self.ui.maskCheckBox.isChecked():
      self.ui.maskWidget.show()
    else:
      self.ui.maskWidget.hide()
    if self.ui.outputCheckBox.isChecked():
      self.ui.outputWidget.show()
      self.ui.maskCheckBox.setEnabled(True)
    else:
      self.ui.outputWidget.hide()
      self.ui.maskCheckBox.setChecked(False)
      self.ui.maskCheckBox.setEnabled(False)
      self.ui.maskWidget.hide()
    
  def selectInput(self):
    fileName = QFileDialog.getOpenFileName(self, "Select Input Image")
    if fileName != "":
      self.ui.inputLineEdit.setText(fileName)
      
  def selectOuput(self):
    fileName = QFileDialog.getSaveFileName(self, "Save Output Image")
    if fileName != "":
      self.ui.outputLineEdit.setText(fileName)
      
  def selectMask(self):
    fileName = QFileDialog.getSaveFileName(self, "Save Output Mask")
    if fileName != "":
      self.ui.maskLineEdit.setText(fileName)
      
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
      
  def setStartButton(self):
    if self.standAlone:
      self.ui.startButton.show()
      self.ui.stopButton.hide()
      self.ui.tabWidget.setTabEnabled(0, True)
      self.ui.tabWidget.setTabEnabled(1, True)
      self.ui.tabWidget.setTabEnabled(2, True)
    
  def setStopButton(self):
    if self.standAlone:
      self.ui.startButton.hide()
      self.ui.stopButton.show()
      self.ui.tabWidget.setTabEnabled(0, False)
      self.ui.tabWidget.setTabEnabled(1, False)
      self.ui.tabWidget.setTabEnabled(2, False)   
	
  def start(self):
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
    self.corrector.start()
  
  def progress(self, value1, value2, string):
    self.ui.totalProgressBar.setValue(value1)
    self.ui.partProgressBar.setValue(value2)
    self.ui.partLabel.setText(string)
    if value1 == 100:
      self.setStartButton()
    
  def log(self, value):
    self.ui.log.append(value)
    
  def badLines(self, values):
    for value in values:
      self.ui.listWidget.addItem(str(value))
  
  def stop(self):
    self.corrector.stop()
    self.setStartButton()
  
  def help(self):
    pass