# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from ui_settings import Ui_Settings

class SettingsDialog(QDialog):

  def __init__(self, parent, settings):
    QDialog.__init__( self, parent )
    self.ui = Ui_Settings()
    self.ui.setupUi( self )
    self.settings = settings
    
    self.connect(self.ui.colorButton, SIGNAL("clicked()"), self.selectCloudColor)
    self.connect(self.ui.dirComboBox, SIGNAL("currentIndexChanged(int)"), self.changeDirChoice)
    self.connect(self.ui.dirButton, SIGNAL("clicked()"), self.openDirectory)
    self.connect(self.ui.buttonBox.button(QDialogButtonBox.Ok), SIGNAL("clicked()"), self.applySettings)
    
    self.loadSettings()
    self.adjustSize()
    
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

  def loadSettings(self):
    dirChoice = int(self.settings.value("cloudMasker/directoryChoice", QVariant(0)).toString())
    self.ui.dirComboBox.setCurrentIndex(dirChoice)
    self.changeDirChoice(dirChoice)
    self.ui.dirLineEdit.setText(self.settings.value("cloudMasker/directoryPath", QVariant("")).toString())
    bandChoice = int(self.settings.value("cloudMasker/bandUse", QVariant(0)).toString())
    self.ui.bandComboBox.setCurrentIndex(bandChoice)
    myColor = self.settings.value("cloudMasker/cloudColor","#008000")
    myColor = QColor( myColor )
    self.ui.colorButton.setColor( myColor )
    self.ui.maskSpinBox.setValue(float(self.settings.value("cloudMasker/unitSize", 1).toString()))
    
  def applySettings(self):
    self.settings.setValue("cloudMasker/directoryChoice", self.ui.dirComboBox.currentIndex())
    self.settings.setValue("cloudMasker/directoryPath", self.ui.dirLineEdit.text())
    self.settings.setValue("cloudMasker/bandUse", self.ui.bandComboBox.currentIndex())
    self.settings.setValue("cloudMasker/cloudColor", self.ui.colorButton.color())
    self.settings.setValue("cloudMasker/unitSize", self.ui.maskSpinBox.value())
    self.close()
    
  def selectCloudColor(self):
    colorDialog = QColorDialog(self)
    colorDialog.setCurrentColor(self.ui.colorButton.color())
    QObject.connect(colorDialog, SIGNAL("colorSelected(const QColor&)"), self.setCloudColor)
    colorDialog.open()
    
  def setCloudColor(self, color):
    self.ui.colorButton.setColor(color)
    
  def changeDirChoice(self, index):
    if index == 2:
      self.ui.widget.show()
    else:
      self.ui.widget.hide()
      
  def openDirectory(self):
    mdir = QFileDialog.getExistingDirectory( self, "Default Directory", "", QFileDialog.ShowDirsOnly )
    if mdir:
      self.ui.dirLineEdit.setText(mdir)
      
  def setBands(self, number):
    self.ui.bandComboBox.clear()
    for i in range(number):
      self.ui.bandComboBox.addItem("Band"+str(i+1))
    bandChoice = int(self.settings.value("cloudMasker/bandUse", QVariant(0)).toString())
    band = 0
    if bandChoice < number and bandChoice >= 0:
      band = bandChoice
    self.ui.bandComboBox.setCurrentIndex(band)
