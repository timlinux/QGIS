#!python
# -*- coding: utf-8 -*-
"""
***************************************************************************************
settings.py - The settings dialog for the SumbandilaSat plug-in's georeferencer.
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
from ui_settings import Ui_Settings

class SettingsDialog(QDialog):

  def __init__(self, parent, settings):
    QDialog.__init__( self, parent )
    self.ui = Ui_Settings()
    self.ui.setupUi( self )
    self.settings = settings
    
    self.connect(self.ui.buttonBox.button(QDialogButtonBox.Ok), SIGNAL("clicked()"), self.applySettings)
    self.connect(self.ui.dirButton, SIGNAL("clicked()"), self.openDirectory)
    self.connect(self.ui.dirComboBox, SIGNAL("currentIndexChanged(int)"), self.changeDirChoice)
    self.connect(self.ui.structureComboBox, SIGNAL("currentIndexChanged(int)"), self.setStructure)
    
    self.loadSettings()
    width = self.width()
    self.adjustSize()
    self.resize(width, self.height())
    
  def loadSettings(self):
    dirChoice = int(self.settings.value("DirectoryChoice", QVariant(0)).toString())
    self.ui.dirComboBox.setCurrentIndex(dirChoice)
    self.ui.dirLineEdit.setText(self.settings.value("DirectoryPath", QVariant("")).toString())
    if dirChoice == 0:
      self.ui.dirWidget.hide()
    elif dirChoice == 1:
      self.ui.dirWidget.show()
    self.ui.resultCheckBox.setChecked(self.settings.value("LoadResult", QVariant(True)).toBool())
    self.ui.acceptCheckBox.setChecked(self.settings.value("AcceptPoints", QVariant(True)).toBool())
    self.ui.backupCheckBox.setChecked(self.settings.value("CreateBackup", QVariant(True)).toBool())
    self.ui.structureComboBox.setCurrentIndex(int(self.settings.value("StructureChoice", QVariant(0)).toString()))
    self.ui.pathLineEdit.setText(self.settings.value("StructurePath", QVariant("")).toString())
    
  def applySettings(self):
    self.settings.setValue("DirectoryChoice", self.ui.dirComboBox.currentIndex())
    self.settings.setValue("DirectoryPath", self.ui.dirLineEdit.text())
    self.settings.setValue("LoadResult", self.ui.resultCheckBox.isChecked())
    self.settings.setValue("AcceptPoints", self.ui.acceptCheckBox.isChecked())
    self.settings.setValue("CreateBackup", self.ui.backupCheckBox.isChecked())
    self.settings.setValue("StructureChoice", self.ui.structureComboBox.currentIndex())
    self.settings.setValue("StructurePath", self.ui.pathLineEdit.text())
    self.close()
    
  def openDirectory(self):
    mdir = QFileDialog.getExistingDirectory( self, "Default Thumbnail Directory", "", QFileDialog.ShowDirsOnly )
    if mdir:
      self.ui.dirLineEdit.setText(mdir)
      
  def changeDirChoice(self, index):
    if index == 0:
      self.ui.dirWidget.hide()
    elif index == 1:
      self.ui.dirWidget.show()
    width = self.width()
    self.adjustSize()
    self.resize(width, self.height())
    
  def setStructure(self, index):
    if index == 0:
      self.ui.pathLineEdit.setText("")
      self.ui.pathLineEdit.setReadOnly(True)
    elif index == 1:
      self.ui.pathLineEdit.setText("../16bit/*.dim")
      self.ui.pathLineEdit.setReadOnly(True)
    elif index == 2:
      self.ui.pathLineEdit.setText("./*.jgw")
      self.ui.pathLineEdit.setReadOnly(True)
    elif index == 3:
      self.ui.pathLineEdit.setReadOnly(False)