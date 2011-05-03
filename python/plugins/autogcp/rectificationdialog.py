#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
rectificationdialogbase.py - The class providing the graphical user interface
for the AutoGCP plugin's rectification dialog. All the necessary information
required to manually orthorectify or geo-transform a raw image with a gcp
set is provided here.
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
from demdialog import *
from ui_rectificationdialogbase import Ui_RectificationDialog

class RectificationDialog(QDialog):
  
  def __init__(self, parent, canvas):
    QDialog.__init__( self, parent )
    self.ui = Ui_RectificationDialog()
    self.ui.setupUi( self )
    self.canvas = canvas
    try:
      QObject.connect(self.ui.outputButton, SIGNAL("clicked()"), self.openOutputFile)
      QObject.connect(self.ui.buttonBox_3, SIGNAL("rejected()"), self.close)
      QObject.connect(self.ui.buttonBox_3, SIGNAL("accepted()"), self.accept)
      QObject.connect(self.ui.buttonBox_3.button(QDialogButtonBox.Help), SIGNAL("clicked()"), self.showHelp)
      QObject.connect(self.ui.driver, SIGNAL("currentIndexChanged(int)"), self.changeDriver)
      QObject.connect(self.ui.comboBox, SIGNAL("currentIndexChanged(int)"), self.changeAlgorithm)
      QObject.connect(self.ui.onlineRadio, SIGNAL("clicked(bool)"), self.selectOnlineDem)

      self.algorithm = 1
      self.outputPath = ""

    except:
      QgsLogger.debug( "Setting up the dialog failed: ", 1, "rectificationdialog.py", "RectificationDialog::__init__()")

  def selectOnlineDem(self, clicked):
    self.demDialog = DemDialog(self, self.canvas)
    QObject.connect(self.demDialog, SIGNAL("demDownloaded(QString)"), self.setDemTempPath)
    self.demDialog.exec_()
    
  def setDemTempPath(self, path):
    self.demDialog.close()
    self.ui.demFile.setText(path)

  def changeAlgorithm(self, i):
    self.algorithm = i

  def getAlgorithm(self):
    return self.algorithm

  def showHelp(self):
    try:
      self.helpDialog = HelpDialog(self)
      self.helpDialog.loadContext("RectificationDialog")
      self.helpDialog.exec_()
    except:
      QgsLogger.debug( "Showing help failed", 1, "rectificationdialog.py", "RectificationDialog::showHelp()")

  def setDriverInfo(self, info):
    try:
      self.info = info
      self.ui.driver.clear()
      for i in range(0, len(self.info)):
        self.ui.driver.addItem(self.info[i].pLongName + " ("+self.info[i].pShortName+")")
      if len(self.info) > 7:
        self.ui.driver.setCurrentIndex(7)
        self.driverName = self.info[7].pShortName
      else:
        self.ui.driver.setCurrentIndex(0)
        self.driverName = self.info[0].pShortName
    except:
      QgsLogger.debug( "Setting the driver information failed", 1, "rectificationdialog.py", "RectificationDialog::setDriverInfo()")

  def changeDriver(self, index):
    try:
      if len(self.info[index].pIssues) > 0:
        self.ui.issueLabel.setVisible(True)
        self.ui.issueText.setVisible(True)
        self.ui.issueText.setPlainText(self.info[index].pIssues)
      else:
        self.ui.issueLabel.setVisible(False)
        self.ui.issueText.setVisible(False)
        #self.ui.issueText.setPlainText("")
      self.driverName = self.info[index].pShortName
    except:
      QgsLogger.debug( "Changing driver failed", 1, "rectificationdialog.py", "RectificationDialog::changeDriver()")

  def getDriverName(self):
    return self.driverName

  def setInformation(self, op):
    try:
      self.outputPath = op
    except:
      QgsLogger.debug( "Setting the output path failed", 1, "rectificationdialog.py", "RectificationDialog::setInformation()")

  def getOutputPath(self):
    return self.outputPath

  def openOutputFile(self):
    try:
      dialog = QFileDialog(None, "Output File", self.outputPath)
      dialog.setFileMode(QFileDialog.AnyFile)
      dialog.setAcceptMode(QFileDialog.AcceptSave)
      if dialog.exec_() == QDialog.Accepted:
        files = dialog.selectedFiles()
        self.outputPath = files[0]
        self.ui.output.setText(self.outputPath)
    except:
      QgsLogger.debug( "Opening output file failed: ", 1, "rectificationdialog.py", "RectificationDialog::openOutputFile()")