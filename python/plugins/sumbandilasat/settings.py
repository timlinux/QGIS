# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from ui_settings import Ui_Settings

class SettingsDialog(QDialog):

  def __init__(self):
    QDialog.__init__(self)
    self.ui = Ui_Settings()
    self.ui.setupUi(self)
    
    QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.RestoreDefaults), SIGNAL("clicked()"), self.restoreDefaults)
    QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Apply), SIGNAL("clicked()"), self.saveSettings)
    QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Cancel), SIGNAL("clicked()"), self.close)
    QObject.connect(self.ui.columnBadCheckBox, SIGNAL("clicked()"), self.toggleColumnCheckBox)
    QObject.connect(self.ui.columnMaskCheckBox, SIGNAL("clicked()"), self.toggleColumnCheckBox)
    QObject.connect(self.ui.lineMaskCheckBox, SIGNAL("clicked()"), self.toggleLineCheckBox)
    QObject.connect(self.ui.alignMethod, SIGNAL("currentIndexChanged(int)"), self.chooseMethod)
    
    self.settings = QSettings("QuantumGIS", "SumbandilaSat Image Processor")
    #No settings exist
    if self.settings.value("init").toInt() != 1:
      self.saveSettings(False)
    else:
      self.loadSettings()
      
    self.toggleColumnCheckBox()
    self.chooseMethod()
    self.toggleLineCheckBox()
  
  def restoreDefaults(self):
    self.ui.general8bitCheckBox.setChecked(True)
    self.ui.generalJpegCheckBox.setChecked(True)
    self.ui.columnBadCheckBox.setChecked(False)
    self.ui.columnMaskCheckBox.setChecked(False)
    self.ui.columnBadLineEdit.setText("*.badColumns")
    self.ui.columnMaskLineEdit.setText("*.columnMask")
    self.ui.columnGain.setValue(0.1)
    self.ui.columnBias.setValue(8.0)
    self.ui.lineMaskCheckBox.setChecked(False)
    self.ui.lineMaskLineEdit.setText("*.lineMask")
    self.ui.linePhase1ComboBox.setCurrentIndex(1)
    self.ui.linePhase2ComboBox.setCurrentIndex(2)
    self.ui.linePhaseCheckBox.setChecked(True)
    self.ui.alignReferenceBand.setCurrentIndex(0)
    self.ui.alignMethod.setCurrentIndex(3)
    self.ui.alignChipSize.setValue(201)
    self.ui.alignGcps.setValue(2048)
    self.ui.alignTolerance.setValue(25)
    self.ui.alignXCheckBox.setChecked(False)
    self.ui.alignYCheckBox.setChecked(False)
    self.toggleColumnCheckBox()
    self.chooseMethod()
    self.toggleLineCheckBox()
  
  def showMessage(self, string, type = "info"):
    msgbox = QMessageBox(self)
    msgbox.setText(string)
    if type == "info":
      msgbox.setWindowTitle("SumbandilaSat Information")
      msgbox.setIcon(QMessageBox.Information)
    elif type == "error":
      msgbox.setWindowTitle("SumbandilaSat Error")
      msgbox.setIcon(QMessageBox.Critical)
    elif type == "warning":
      msgbox.setWindowTitle("SumbandilaSat Warning")
      msgbox.setIcon(QMessageBox.Warning)
    elif type == "question":
      msgbox.setWindowTitle("SumbandilaSat Question")
      msgbox.setIcon(QMessageBox.Question)
    elif type == "none":
      msgbox.setWindowTitle("SumbandilaSat")
      msgbox.setIcon(QMessageBox.NoIcon)
    msgbox.setModal(True)
    msgbox.exec_() 
   
  #varType
  #0: string
  #1: bool
  #2: int
  #3: double
  def value(self, category, name, varType):
    if varType == 0:
      var = self.settings.value(category+"/"+name, QVariant(""))
      return var.toString()
    if varType == 1:
      var = self.settings.value(category+"/"+name, QVariant(False))
      return var.toBool()
    if varType == 2:
      var = self.settings.value(category+"/"+name, QVariant(0))
      return int(var.toString())
    if varType == 3:
      var = self.settings.value(category+"/"+name, QVariant(0.0))
      return float(var.toString())
    
  def loadSettings(self):
    #General
    self.ui.general8bitCheckBox.setChecked(self.value("general", "create8bit", 1))
    self.ui.generalJpegCheckBox.setChecked(self.value("general", "createJpeg", 1))
    
    #Column correction
    self.ui.columnBadCheckBox.setChecked(self.value("columnCorrection", "exportBadColumns", 1))
    self.ui.columnMaskCheckBox.setChecked(self.value("columnCorrection", "createMask", 1))
    self.ui.columnBadLineEdit.setText(self.value("columnCorrection", "badExtension", 0))
    self.ui.columnMaskLineEdit.setText(self.value("columnCorrection", "maskExtension", 0))
    self.ui.columnGain.setValue(self.value("columnCorrection", "gainThreshold", 3))
    self.ui.columnBias.setValue(self.value("columnCorrection", "biasThreshold", 3))
    self.toggleColumnCheckBox()
    
    #Line correction
    self.ui.lineMaskCheckBox.setChecked(self.value("lineCorrection", "createMask", 1))
    self.ui.lineMaskLineEdit.setText(self.value("lineCorrection", "maskExtension", 0))
    self.ui.linePhase1ComboBox.setCurrentIndex(self.value("lineCorrection", "phase1", 2))
    self.ui.linePhase2ComboBox.setCurrentIndex(self.value("lineCorrection", "phase2", 2))
    self.ui.linePhaseCheckBox.setChecked(self.value("lineCorrection", "recalculate", 1))
    self.toggleLineCheckBox()
    
    #Band alignment
    self.ui.alignReferenceBand.setCurrentIndex(self.value("bandAlignment", "referenceBand", 2)-1)
    self.ui.alignMethod.setCurrentIndex(self.value("bandAlignment", "method", 2))
    self.ui.alignChipSize.setValue(self.value("bandAlignment", "chipSize", 2))
    self.ui.alignGcps.setValue(self.value("bandAlignment", "minimumGcps", 2))
    self.ui.alignTolerance.setValue(self.value("bandAlignment", "refinementTolerance", 3))
    self.ui.alignXCheckBox.setChecked(self.value("lineCorrection", "createXMap", 1))
    self.ui.alignYCheckBox.setChecked(self.value("lineCorrection", "createYMap", 1))
    self.chooseMethod()
    
  def saveSettings(self, close = True):
    #General
    self.settings.setValue("general/create8bit", self.ui.general8bitCheckBox.isChecked())
    self.settings.setValue("general/createJpeg", self.ui.generalJpegCheckBox.isChecked())
    
    #Column correction
    if (self.ui.columnBadCheckBox.isChecked() and self.ui.columnBadLineEdit.text() == ""):
      self.showMessage("Please provide a file extension for the column correction's bad column export file.")
      return
    if (self.ui.columnMaskCheckBox.isChecked() and self.ui.columnMaskLineEdit.text() == ""):
      self.showMessage("Please provide a file extension for the column correction's mask file.")
      return
    self.settings.setValue("columnCorrection/exportBadColumns", self.ui.columnBadCheckBox.isChecked())
    self.settings.setValue("columnCorrection/createMask", self.ui.columnMaskCheckBox.isChecked())
    self.settings.setValue("columnCorrection/badExtension", self.ui.columnBadLineEdit.text())
    self.settings.setValue("columnCorrection/maskExtension", self.ui.columnMaskLineEdit.text())
    self.settings.setValue("columnCorrection/gainThreshold", self.ui.columnGain.value())
    self.settings.setValue("columnCorrection/biasThreshold", self.ui.columnBias.value())
    
    #Line correction
    if (self.ui.lineMaskCheckBox.isChecked() and self.ui.lineMaskLineEdit.text() == ""):
      self.showMessage("Please provide a file extension for the line correction's mask file.")
      return
    self.settings.setValue("lineCorrection/createMask", self.ui.lineMaskCheckBox.isChecked())
    self.settings.setValue("lineCorrection/maskExtension", self.ui.lineMaskLineEdit.text())
    self.settings.setValue("lineCorrection/phase1", self.ui.linePhase1ComboBox.currentIndex())
    self.settings.setValue("lineCorrection/phase2", self.ui.linePhase2ComboBox.currentIndex())
    self.settings.setValue("lineCorrection/recalculate", self.ui.linePhaseCheckBox.isChecked())
    
    #Band alignment
    self.settings.setValue("bandAlignment/referenceBand", self.ui.alignReferenceBand.currentIndex()+1)
    self.settings.setValue("bandAlignment/method", self.ui.alignMethod.currentIndex())
    self.settings.setValue("bandAlignment/chipSize", self.ui.alignChipSize.value())
    self.settings.setValue("bandAlignment/minimumGcps", self.ui.alignGcps.value())
    self.settings.setValue("bandAlignment/refinementTolerance", self.ui.alignTolerance.value())
    self.settings.setValue("bandAlignment/createXMap", self.ui.alignXCheckBox.isChecked())
    self.settings.setValue("bandAlignment/createYMap", self.ui.alignYCheckBox.isChecked())
    
    #Mark as created
    self.settings.setValue("init", 1)
    
    if close:
      self.close()
    
  def chooseMethod(self, index = -1):
    index = self.ui.alignMethod.currentIndex()
    if index == 0:
      self.ui.disparityGroupBox.show()
    else:
      self.ui.disparityGroupBox.hide()
    
  def toggleLineCheckBox(self):
    if self.ui.lineMaskCheckBox.isChecked():
      self.ui.lineMaskWidget.show()
    else:
      self.ui.lineMaskWidget.hide()
   
  def toggleColumnCheckBox(self):
    if self.ui.columnBadCheckBox.isChecked():
      self.ui.columnBadWidget.show()
    else:
      self.ui.columnBadWidget.hide()
    if self.ui.columnMaskCheckBox.isChecked():
      self.ui.columnMaskWidget.show()
    else:
      self.ui.columnMaskWidget.hide()
      