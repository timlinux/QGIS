#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
databasedialogbase.py - The class providing the graphical user interface for
the AutoGCP plugin's Database dialog. All the necessary information
required to manually query the PostgreSQL and SQLite database for GCPs and
GCP chips based on locations, images or hashes is provided here.
--------------------------------------
Date : 21-September-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
Modified: Christoph Stallmann
Modification Date: 21/09/2010
/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
"""
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from helpviewer import *
from qgis.core import *
from ui_databasedialogbase import Ui_DatabaseDialog

class DatabaseDialog(QDialog):
  
  def __init__(self, parent):
    QDialog.__init__( self, parent )
    self.ui = Ui_DatabaseDialog()
    self.ui.setupUi( self )
    try:
      QObject.connect(self.ui.defaultRadio, SIGNAL("clicked()"), self.changeLoadingOption)
      QObject.connect(self.ui.hashRadio, SIGNAL("clicked()"), self.changeLoadingOption)
      QObject.connect(self.ui.checkRef, SIGNAL("clicked()"), self.changeSelection)
      QObject.connect(self.ui.checkRaw, SIGNAL("clicked()"), self.changeSelection)
      QObject.connect(self.ui.buttonBox, SIGNAL("rejected()"), self.close)
      QObject.connect(self.ui.buttonBox, SIGNAL("accepted()"), self.saveParameters)
      QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Help), SIGNAL("clicked()"), self.showHelp)

      self.dialogPointer = DatabaseDialog
      self.groupBox_2.setVisible(False)
      self.dialogPointer.adjustSize()
      self.centerDialog()
      self.loadingOption = 0
      self.imageHash = ""
      self.projectionString = "proj"
    except:
      QgsLogger.debug( "Setting up the dialog failed", 1, "databasedialog.py", "DatabaseDialog::__init__()")
      
  def changeSelection(self):
    try:
      if self.checkRaw.isChecked():
	self.checkRef.setChecked(True)
    except:
      QgsLogger.debug( "Changing selection failed", 1, "databasedialog.py", "DatabaseDialog::changeSelection()")

  def showHelp(self):
    try:
      self.helpDialog = HelpDialog(self)
      self.helpDialog.loadContext("DatabaseDialog")
      self.helpDialog.exec_()
    except:
      QgsLogger.debug( "Showing help failed", 1, "databasedialog.py", "DatabaseDialog::showHelp()")

  def setFilter(self, selection):
    try:
      if selection == 0:
	self.checkRef.setChecked(True)
      else:
	self.checkRef.setChecked(True)
        self.checkRaw.setChecked(True)
    except:
      QgsLogger.debug( "Setting filter failed", 1, "databasedialog.py", "DatabaseDialog::setFilter()")

  def getFilter(self):
    try:
      if self.checkRef.isChecked() and self.checkRaw.isChecked():
	return 2
      elif self.checkRef.isChecked():
	return 0
      elif self.checkRaw.isChecked():
	return 1
    except:
      QgsLogger.debug( "Getting filter failed", 1, "databasedialog.py", "DatabaseDialog::getFilter()")
      return -1

  def getLoadingOption(self):
    return self.loadingOption

  def getImageHash(self):
    return self.imageHash

  def saveParameters(self):
    try:
      if self.defaultRadio.isChecked():
	self.loadingOption == 0
      elif self.hashRadio.isChecked():
	self.loadingOption = 1
      elif self.locationRadio.isChecked():
	self.loadingOption = 2
      if self.loadingOption == 1:
	try:
	  self.imageHash = self.lineEdit.text()
        except:
	  msgbox = QtGui.QMessageBox()
          msgbox.setText("A format error was detected in one of the inputs.")
          msgbox.setWindowTitle("Format Error")
          msgbox.setIcon(QMessageBox.Warning)
          msgbox.setModal(True)
          msgbox.exec_()
      self.dialogPointer.accept()
    except:
      QgsLogger.debug( "Saving parameters failed", 1, "databasedialog.py", "DatabaseDialog::saveParameters()")

  def changeLoadingOption(self):
    try:
      if self.defaultRadio.isChecked():
	self.groupBox_4.setVisible(True)
	self.groupBox_2.setVisible(False)
      elif self.hashRadio.isChecked():
	self.groupBox_4.setVisible(False)
        self.groupBox_2.setVisible(True)
      self.centerDialog()
    except:
      QgsLogger.debug( "Changing loading options failed", 1, "databasedialog.py", "DatabaseDialog::changeLoadingOption()")

  def centerDialog(self):
    try:
      screen = QtGui.QApplication.desktop()
      width = self.dialogPointer.width()
      height = self.dialogPointer.height()
      x = (screen.screenGeometry(screen.primaryScreen()).width() - width) / 2
      y = (screen.screenGeometry(screen.primaryScreen()).height() - height) / 2
      self.dialogPointer.move(x, y)
    except:
      QgsLogger.debug( "Centering the dialog failed", 1, "databasedialog.py", "DatabaseDialog::centerDialog()")