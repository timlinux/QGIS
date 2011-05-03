#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
settingsdialogbase.py - The class providing the graphical user interface
for the AutoGCP plugin's Settings dialog. All the necessary information used
by the user to manually tweak the user interface and algorithms is provided
here.
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
from qgis.core import *
from qgis.gui import QgsColorButton
from settings import *
from xmlhandler import *
from helpviewer import *
from ui_settingsdialogbase import Ui_SettingsDialog

class SettingsDialog(QDialog):
  
  def __init__(self, parent):
    QDialog.__init__( self, parent )
    self.ui = Ui_SettingsDialog()
    self.ui.setupUi( self )
    try:
      view = self.ui.markerIcon.view()
      model = view.model()
      model.setData(model.index(0, 0), QVariant(Qt.AlignCenter), Qt.TextAlignmentRole)

      self.ui.rawCanvasColor.setPalette(QPalette(Qt.black))
      self.ui.refCanvasColor.setPalette(QPalette(Qt.black))

      self.dir = QDir(QgsApplication.qgisSettingsDirPath())
      self.dir.mkdir("python")
      self.dir.setPath(self.dir.absolutePath()+"/python")
      self.dir.mkdir("plugins")
      self.dir.setPath(self.dir.absolutePath()+"/plugins")
      self.dir.mkdir("autogcp")
      self.dir.setPath(self.dir.absolutePath()+"/autogcp/")
      self.xmlHandler = AutoGcpXmlHandler(QgsApplication.qgisSettingsDirPath() + "python/plugins/autogcp/settings.xml")

      self.ui.refDialogPath.setVisible(False)
      self.ui.refDialogButton.setVisible(False)
      self.ui.rawDialogPath.setVisible(False)
      self.ui.rawDialogButton.setVisible(False)
      self.ui.sqliteGroupBox.setVisible(False)
      self.ui.tabWidget.setCurrentIndex(0)

      QObject.connect(self.ui.buttonBox, SIGNAL("accepted()"), self.saveSettings)
      QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Help), SIGNAL("clicked()"), self.showHelp)
      QObject.connect(self.ui.buttonBox, SIGNAL("rejected()"), self.close)
      QObject.connect(self.ui.markerIcon, SIGNAL("currentIndexChanged(int)"), self.selectMarkerIcon)
      QObject.connect(self.ui.selectedIcon, SIGNAL("currentIndexChanged(int)"), self.selectSelectedIcon)
      QObject.connect(self.ui.refCanvasColor, SIGNAL("clicked()"), self.selectRefCanvasColor)
      QObject.connect(self.ui.rawCanvasColor, SIGNAL("clicked()"), self.selectRawCanvasColor)
      QObject.connect(self.ui.gcpAmount, SIGNAL("valueChanged(int)"), self.changeGcpAmount)
      QObject.connect(self.ui.bandSelection, SIGNAL("currentIndexChanged(int)"), self.selectBand)
      QObject.connect(self.ui.chipWidth, SIGNAL("valueChanged(int)"), self.changeChipWidth)
      QObject.connect(self.ui.chipHeight, SIGNAL("valueChanged(int)"), self.changeChipHeight)
      QObject.connect(self.ui.refDialogBox, SIGNAL("currentIndexChanged(int)"), self.changeRefDialogChoice)
      QObject.connect(self.ui.rawDialogBox, SIGNAL("currentIndexChanged(int)"), self.changeRawDialogChoice)
      QObject.connect(self.ui.refDialogButton, SIGNAL("clicked()"), self.showRefChooser)
      QObject.connect(self.ui.rawDialogButton, SIGNAL("clicked()"), self.showRawChooser)
      QObject.connect(self.ui.radioNone, SIGNAL("clicked()"), self.changeNoneSelection)
      QObject.connect(self.ui.radioSqlite, SIGNAL("clicked()"), self.changeSqliteSelection)
      QObject.connect(self.ui.loadingBox, SIGNAL("currentIndexChanged(int)"), self.changeLoadingChoice)
      QObject.connect(self.ui.sqliteBox, SIGNAL("currentIndexChanged(int)"), self.changeSqliteChoice)
      QObject.connect(self.ui.sqliteButton, SIGNAL("clicked()"), self.showSqliteChooser)
      QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.RestoreDefaults), SIGNAL("clicked()"), self.restoreDefaults)

      self.databaseChanged = False
      self.setDefaults()
      self.loadSettings()
    except:
      QgsLogger.debug( "Setting up the dialog failed: ", 1, "settingsdialog.py", "SettingsDialog::__init__()")

  def showHelp(self):
    try:
      self.helpDialog = HelpDialog(self)
      self.helpDialog.loadContext("SettingsDialog")
      self.helpDialog.exec_()
    except:
      QgsLogger.debug( "Showing help failed: ", 1, "settingsdialog.py", "SettingsDialog::showHelp()")

  def setUserSettings(self, obj):
    try:
      self.settings = obj
    except:
      QgsLogger.debug( "Setting user settings failed", 1, "settingsdialog.py", "SettingsDialog::setUserSettings()")

  def setBandCount(self, bands):
    try:
      self.ui.bandSelection.clear()
      for i in range(0, bands):
        self.ui.bandSelection.addItem(str(i+1))
    except:
      QgsLogger.debug( "Setting the band count failed", 1, "settingsdialog.py", "SettingsDialog::setBandCount()")

  def restoreDefaults(self):
    msgbox = QtGui.QMessageBox(self)
    msgbox.setText("Are you sure you want to restore all settings to their default values?")
    msgbox.setWindowTitle("Restore Defaults")
    msgbox.setIcon(QMessageBox.Question)
    msgbox.setModal(True)
    msgbox.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
    msgbox.setDefaultButton(QMessageBox.No)
    res = msgbox.exec_()
    if res == QMessageBox.Yes:
      self.setDefaults()
      self.loadSettings(False)
      self.saveSettings(False)
      self.emit(SIGNAL("accepted()"))

  def setDefaults(self):
    try:
      self.tempSettings = AutoGcpSettings()
      self.tempSettings.refPathChoice = 1
      self.tempSettings.refPath = ""
      self.tempSettings.rawPathChoice = 1
      self.tempSettings.rawPath = ""
      self.tempSettings.lastOpenPath = ""
      self.tempSettings.markerIconType = 1
      self.tempSettings.markerIconColor = 0
      self.tempSettings.selectedIconType = 1
      self.tempSettings.selectedIconColor = 1
      self.tempSettings.refCanvasColor = Qt.black
      self.tempSettings.rawCanvasColor = Qt.black
      self.tempSettings.gcpAmount = 30
      self.tempSettings.correlationThreshold = 0.85
      self.tempSettings.bandSelection = 1
      self.tempSettings.chipWidth = 32
      self.tempSettings.chipHeight = 32
      self.tempSettings.rmsError = 0.5
      self.tempSettings.databaseSelection = 0
      self.ui.sqliteGroupBox.hide()
      self.tempSettings.gcpLoading = 1
      self.tempSettings.sqliteChoice = 0
      self.tempSettings.sqlitePath = ""
    except:
      QgsLogger.debug( "Setting the setting defaults failed", 1, "settingsdialog.py", "SettingsDialog::setDefaults()")

  def loadSettings(self, loadFromSaveSettings = True):
    try:
      if loadFromSaveSettings:
	self.xmlHandler.setSettings(self.tempSettings)
	self.xmlHandler.openXml()

      self.ui.refDialogBox.setCurrentIndex(self.tempSettings.refPathChoice)
      if self.tempSettings.refPathChoice == 0 or self.tempSettings.refPathChoice == 1:
        self.ui.refDialogPath.setVisible(False)
        self.ui.refDialogButton.setVisible(False)
      else:
        self.ui.refDialogPath.setVisible(True)
        self.ui.refDialogButton.setVisible(True)
      self.ui.refDialogPath.setText(self.tempSettings.refPath)
      self.ui.rawDialogBox.setCurrentIndex(self.tempSettings.rawPathChoice)
      if self.tempSettings.rawPathChoice == 0 or self.tempSettings.rawPathChoice == 1:
        self.ui.rawDialogPath.setVisible(False)
        self.ui.rawDialogButton.setVisible(False)
      else:
        self.ui.rawDialogPath.setVisible(True)
        self.ui.rawDialogButton.setVisible(True)
      self.ui.rawDialogPath.setText(self.tempSettings.rawPath)

      self.ui.gcpAmount.setValue(self.tempSettings.gcpAmount)
      try:
        self.ui.bandSelection.setCurrentIndex(self.tempSettings.bandSelection-1)
      except:
        self.ui.bandSelection.setCurrentIndex(0)
      self.ui.correlationThreshold.setValue(self.tempSettings.correlationThreshold)
      self.ui.chipWidth.setValue(self.tempSettings.chipWidth)
      self.ui.chipHeight.setValue(self.tempSettings.chipHeight)
      self.ui.rms.setValue(self.tempSettings.rmsError)

      self.ui.refCanvasColor.setColor(self.tempSettings.refCanvasColor)
      self.ui.rawCanvasColor.setColor(self.tempSettings.rawCanvasColor)
      selectedIndex = self.tempSettings.selectedIconColor # because this value gets change if the normal GCP marker is changed
      if self.tempSettings.markerIconType == 0:
        if self.tempSettings.markerIconColor == 0:
          self.ui.markerIcon.setCurrentIndex(6)
        elif self.tempSettings.markerIconColor == 1:
          self.ui.markerIcon.setCurrentIndex(5)
        elif self.tempSettings.markerIconColor == 2:
          self.ui.markerIcon.setCurrentIndex(4)
        elif self.tempSettings.markerIconColor == 3:
          self.ui.markerIcon.setCurrentIndex(7)
      elif self.tempSettings.markerIconType == 1:
        if self.tempSettings.markerIconColor == 0:
          self.ui.markerIcon.setCurrentIndex(2)
        elif self.tempSettings.markerIconColor == 1:
          self.ui.markerIcon.setCurrentIndex(1)
        elif self.tempSettings.markerIconColor == 2:
          self.ui.markerIcon.setCurrentIndex(0)
        elif self.tempSettings.markerIconColor == 3:
          self.ui.markerIcon.setCurrentIndex(3)
      self.tempSettings.selectedIconColor = selectedIndex
      if selectedIndex == 0:
        self.ui.selectedIcon.setCurrentIndex(2)
      elif selectedIndex == 1:
        self.ui.selectedIcon.setCurrentIndex(1)
      elif selectedIndex == 2:
        self.ui.selectedIcon.setCurrentIndex(0)
      elif selectedIndex == 3:
        self.ui.selectedIcon.setCurrentIndex(3)
      if self.tempSettings.loadGcpTable == 0:
        self.ui.checkBox.setChecked(False)
      else:
        self.ui.checkBox.setChecked(True)
      if self.tempSettings.loadRefCanvas == 0:
        self.ui.checkBox_2.setChecked(False)
      else:
        self.ui.checkBox_2.setChecked(True)
      if self.tempSettings.loadRawCanvas == 0:
        self.ui.checkBox_3.setChecked(False)
      else:
        self.ui.checkBox_3.setChecked(True)

      if self.tempSettings.databaseSelection == 0:
        self.ui.radioNone.setChecked(True)
        self.ui.sqliteGroupBox.setVisible(False)
      elif self.tempSettings.databaseSelection == 1:
        self.ui.radioSqlite.setChecked(True)
        self.ui.sqliteGroupBox.setVisible(True)
      self.ui.loadingBox.setCurrentIndex(self.tempSettings.gcpLoading)
      if self.tempSettings.sqliteChoice == 0:
        self.ui.sqliteBox.setCurrentIndex(0)
        self.ui.sqlitePath.setVisible(False)
        self.ui.sqliteButton.setVisible(False)
      else:
        self.ui.sqliteBox.setCurrentIndex(1)
        self.ui.sqlitePath.setVisible(True)
        self.ui.sqliteButton.setVisible(True)
      self.ui.sqlitePath.setText(self.tempSettings.sqlitePath)
    except:
      QgsLogger.debug( "Loading the settings failed", 1, "settingsdialog.py", "SettingsDialog::loadSettings()")

  def saveSettings(self, exit = True):
    try:
      if (not self.tempSettings.sqlitePath == self.ui.sqlitePath.text()):
        self.databaseChanged = True
      self.tempSettings.refPath = self.ui.refDialogPath.text()
      self.tempSettings.rawPath = self.ui.rawDialogPath.text()
      self.settings.refPath = self.tempSettings.refPath
      self.settings.rawPath = self.tempSettings.rawPath
      self.settings.refPathChoice = self.tempSettings.refPathChoice
      self.settings.rawPathChoice = self.tempSettings.rawPathChoice
      self.settings.lastOpenPath = self.tempSettings.lastOpenPath
      self.settings.markerIconType = self.tempSettings.markerIconType
      self.settings.markerIconColor = self.tempSettings.markerIconColor
      self.settings.selectedIconType = self.tempSettings.selectedIconType
      self.settings.selectedIconColor = self.tempSettings.selectedIconColor
      self.settings.refCanvasColor = self.tempSettings.refCanvasColor
      self.settings.rawCanvasColor = self.tempSettings.rawCanvasColor
      if self.ui.checkBox.isChecked():
        self.settings.loadGcpTable = 1
      else:
        self.settings.loadGcpTable = 0
      if self.ui.checkBox_2.isChecked():
        self.settings.loadRefCanvas = 1
      else:
        self.settings.loadRefCanvas = 0
      if self.ui.checkBox_3.isChecked():
        self.settings.loadRawCanvas = 1
      else:
        self.settings.loadRawCanvas = 0
      self.settings.gcpAmount = self.tempSettings.gcpAmount
      self.settings.bandSelection = self.tempSettings.bandSelection
      self.settings.correlationThreshold = self.ui.correlationThreshold.value()
      self.settings.chipWidth = self.tempSettings.chipWidth
      self.settings.chipHeight= self.tempSettings.chipHeight
      self.settings.rmsError = self.ui.rms.value()
      self.tempSettings.sqlitePath = self.ui.sqlitePath.text()
      self.settings.databaseSelection = self.tempSettings.databaseSelection
      self.settings.gcpLoading = self.tempSettings.gcpLoading
      self.settings.sqliteChoice = self.tempSettings.sqliteChoice
      self.settings.sqlitePath = self.tempSettings.sqlitePath
      self.xmlHandler.setSettings(self.settings)
      self.xmlHandler.saveXml()
      if self.databaseChanged:
        msgbox = QtGui.QMessageBox(self)
        msgbox.setText("The database settings were changed.\nPlease restart QGis to apply these settings.")
        msgbox.setWindowTitle("Database Changed")
        msgbox.setIcon(QMessageBox.Information)
        msgbox.setModal(True)
        msgbox.exec_()
      if exit:
	self.accept()
    except:
      QgsLogger.debug( "Saving the settings failed", 1, "settingsdialog.py", "SettingsDialog::saveSettings()")

  def changeLoadingChoice(self, index):
    try:
      self.tempSettings.gcpLoading = index
    except:
      QgsLogger.debug( "Changing the loading choice failed", 1, "settingsdialog.py", "SettingsDialog::changeLoadingChoice()")

  def showSqliteChooser(self):
    try:
      fileDir = QFileDialog.getSaveFileName(self, "SQLite Database", self.dir.absolutePath())
      self.ui.sqlitePath.setText(fileDir)
    except:
      QgsLogger.debug( "Showing SQLite file dialog failed", 1, "settingsdialog.py", "SettingsDialog::showSqliteChooser()")

  def changeNoneSelection(self):
    try:
      if self.ui.radioNone.isChecked():
        self.tempSettings.databaseSelection = 0
        self.ui.sqliteGroupBox.setVisible(False)
        self.databaseChanged = True
        self.ui.loadingBox.hide()
        self.ui.label_17.hide()
    except:
      QgsLogger.debug( "Changing to None selection failed", 1, "settingsdialog.py", "SettingsDialog::changeNoneSelection()")

  def changeSqliteSelection(self):
    try:
      if self.ui.radioSqlite.isChecked():
        self.tempSettings.databaseSelection = 1
        self.ui.sqliteGroupBox.setVisible(True)
        self.databaseChanged = True
        self.ui.loadingBox.show()
        self.ui.label_17.show()
    except:
      QgsLogger.debug( "Changing to SQLite selection failed", 1, "settingsdialog.py", "SettingsDialog::changeSqliteSelection()")

  def changeSqliteChoice(self, index):
    try:
      self.tempSettings.sqliteChoice = index
      if index == 0:
        self.ui.sqlitePath.setVisible(False)
        self.ui.sqliteButton.setVisible(False)
      else:
        self.ui.sqlitePath.setVisible(True)
        self.ui.sqliteButton.setVisible(True)
      self.databaseChanged = True
    except:
      QgsLogger.debug( "Changing the SQLite choice failed", 1, "settingsdialog.py", "SettingsDialog::changeSqliteChoice()")

  def showRefChooser(self):
    try:
      fileDir = QFileDialog.getExistingDirectory(self, "Reference Image Default Directory", self.tempSettings.refPath)
      self.ui.refDialogPath.setText(fileDir)
    except:
      QgsLogger.debug( "Showing the reference image default file dialog failed", 1, "settingsdialog.py", "SettingsDialog::showRefChooser()")

  def showRawChooser(self):
    try:
      fileDir = QFileDialog.getExistingDirectory(self, "Raw Image Default Directory", self.tempSettings.rawPath)
      self.ui.rawDialogPath.setText(fileDir)
    except:
      QgsLogger.debug( "Showing the raw image default file dialog failed", 1, "settingsdialog.py", "SettingsDialog::showRawChooser()")

  def changeRefDialogChoice(self, index):
    try:
      self.tempSettings.refPathChoice = index
      if index == 0:
        self.ui.refDialogPath.setVisible(False)
        self.ui.refDialogButton.setVisible(False)
      elif index == 1:
        self.ui.refDialogPath.setVisible(False)
        self.ui.refDialogButton.setVisible(False)
      else:
        self.ui.refDialogPath.setVisible(True)
        self.ui.refDialogButton.setVisible(True)
    except:
      QgsLogger.debug( "Changing the reference dialog choice failed", 1, "settingsdialog.py", "SettingsDialog::changeRefDialogChoice()")

  def changeRawDialogChoice(self, index):
    try:
      self.tempSettings.rawPathChoice = index
      if index == 0:
        self.ui.rawDialogPath.setVisible(False)
        self.ui.rawDialogButton.setVisible(False)
      elif index == 1:
        self.ui.rawDialogPath.setVisible(False)
        self.ui.rawDialogButton.setVisible(False)
      else:
        self.ui.rawDialogPath.setVisible(True)
        self.ui.rawDialogButton.setVisible(True)
    except:
      QgsLogger.debug( "Changing the raw dialog choice failed", 1, "settingsdialog.py", "SettingsDialog::changeRawDialogChoice()")

  def selectRefCanvasColor(self):
    try:
      colorDialog = QColorDialog(self)
      colorDialog.setCurrentColor(self.tempSettings.refCanvasColor)
      QObject.connect(colorDialog, SIGNAL("colorSelected(const QColor &)"), self.setRefCanvasColor)
      colorDialog.open()
    except:
      QgsLogger.debug( "Selecting the reference canvas color failed", 1, "settingsdialog.py", "SettingsDialog::selectRefCanvasColor()")

  def setRefCanvasColor(self, color):
    try:
      self.tempSettings.refCanvasColor = color
      self.ui.refCanvasColor.setColor(self.tempSettings.refCanvasColor)
    except:
      QgsLogger.debug( "Setting the reference canvas color failed", 1, "settingsdialog.py", "SettingsDialog::setRefCanvasColor()")

  def selectRawCanvasColor(self):
    try:
      colorDialog = QColorDialog(self)
      colorDialog.setCurrentColor(self.tempSettings.rawCanvasColor)
      QObject.connect(colorDialog, SIGNAL("colorSelected(const QColor &)"), self.setRawCanvasColor)
      colorDialog.open()
    except:
      QgsLogger.debug( "Selecting the raw canvas color failed", 1, "settingsdialog.py", "SettingsDialog::selectRawCanvasColor()")

  def setRawCanvasColor(self, color):
    try:
      self.tempSettings.rawCanvasColor = color
      self.ui.rawCanvasColor.setColor(self.tempSettings.rawCanvasColor)
    except:
      QgsLogger.debug( "Setting the raw canvas color failed", 1, "settingsdialog.py", "SettingsDialog::setRawCanvasColor()")

  def selectSelectedIcon(self, index):
    try:
      if index == 0:
        self.tempSettings.selectedIconColor = 2
      elif index == 1:
        self.tempSettings.selectedIconColor = 1
      elif index == 2:
        self.tempSettings.selectedIconColor = 0
      elif index == 3:
        self.tempSettings.selectedIconColor = 3
    except:
      QgsLogger.debug( "Selecting GCP highlight icon failed", 1, "settingsdialog.py", "SettingsDialog::selectSelectedIcon()")

  def selectMarkerIcon(self, index):
    try:
      if index < 4:
        self.tempSettings.markerIconType = 1
        self.tempSettings.selectedIconType = 1
        if index == 0:
          self.tempSettings.markerIconColor = 2
        elif index == 1:
          self.tempSettings.markerIconColor = 1
        elif index == 2:
          self.tempSettings.markerIconColor = 0
        elif index == 3:
          self.tempSettings.markerIconColor = 3
        self.ui.selectedIcon.clear()
        icon0 = QtGui.QIcon()
        icon0.addPixmap(QtGui.QPixmap(":/icons/pinblue.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.selectedIcon.addItem(icon0, "")
        self.ui.selectedIcon.setItemText(0, "")
        icon1 = QtGui.QIcon()
        icon1.addPixmap(QtGui.QPixmap(":/icons/pingreen.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.selectedIcon.addItem(icon1, "")
        self.ui.selectedIcon.setItemText(1, "")
        icon2 = QtGui.QIcon()
        icon2.addPixmap(QtGui.QPixmap(":/icons/pinred.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.selectedIcon.addItem(icon2, "")
        self.ui.selectedIcon.setItemText(2, "")
        icon3 = QtGui.QIcon()
        icon3.addPixmap(QtGui.QPixmap(":/icons/pinyellow.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.selectedIcon.addItem(icon3, "")
        self.ui.selectedIcon.setItemText(3, "")
      elif index > 3 and index < 8:
        self.tempSettings.markerIconType = 0
        self.tempSettings.selectedIconType = 0
        if index == 4:
          self.tempSettings.markerIconColor = 2
        elif index == 5:
          self.tempSettings.markerIconColor = 1
        elif index == 6:
          self.tempSettings.markerIconColor = 0
        elif index == 7:
          self.tempSettings.markerIconColor = 3
        self.ui.selectedIcon.clear()
        icon0 = QtGui.QIcon()
        icon0.addPixmap(QtGui.QPixmap(":/icons/flagblue.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.selectedIcon.addItem(icon0, "")
        self.ui.selectedIcon.setItemText(0, "")
        icon1 = QtGui.QIcon()
        icon1.addPixmap(QtGui.QPixmap(":/icons/flaggreen.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.selectedIcon.addItem(icon1, "")
        self.ui.selectedIcon.setItemText(1, "")
        icon2 = QtGui.QIcon()
        icon2.addPixmap(QtGui.QPixmap(":/icons/flagred.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.selectedIcon.addItem(icon2, "")
        self.ui.selectedIcon.setItemText(2, "")
        icon3 = QtGui.QIcon()
        icon3.addPixmap(QtGui.QPixmap(":/icons/flagyellow.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        self.ui.selectedIcon.addItem(icon3, "")
        self.ui.selectedIcon.setItemText(3, "")
    except:
      QgsLogger.debug( "Selecting GCP icon failed", 1, "settingsdialog.py", "SettingsDialog::selectMarkerIcon()")

  def changeGcpAmount(self, val):
    try:
      self.tempSettings.gcpAmount = val
    except:
      QgsLogger.debug( "Changing the GCP amount failed", 1, "settingsdialog.py", "SettingsDialog::changeGcpAmount()")

  def selectBand(self, val):
    try:
      self.tempSettings.bandSelection = (val+1)
    except:
      QgsLogger.debug( "Selecting a band failed", 1, "settingsdialog.py", "SettingsDialog::selectBand()")

  def changeChipWidth(self, val):
    try:
      self.tempSettings.chipWidth = val
    except:
      QgsLogger.debug( "Changing chip width failed", 1, "settingsdialog.py", "SettingsDialog::changeChipWidth()")

  def changeChipHeight(self, val):
    try:
      self.tempSettings.chipHeight = val
    except:
      QgsLogger.debug( "Changing chip height failed", 1, "settingsdialog.py", "SettingsDialog::changeChipHeight()")