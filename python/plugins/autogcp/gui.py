#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
guibase.py - This class is the main driver behind the AutoGCP plugin. All
functionality is managed and controled here. This class also provides the
major design behind the graphical user interface.
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
#FoxHat Added - Start
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from qgis.analysis import *
from settingsdialog import *
from projectiondialog import *
from aboutdialog import *
from imagedialog import *
from settings import *
from xmlhandler import *
from gcpitem import *
from databasedialog import *
from rectificationdialog import *
from progressdialog import *

import copy, time
from ui_guibase import Ui_MainWindow

class MainWindow(QMainWindow):

  def __init__(self, iface):
    QMainWindow.__init__( self )
    self.ui = Ui_MainWindow()
    self.iface = iface
    self.ui.setupUi( self )
    #ensure that the dock widgets resize correctly
    self.setCentralWidget(None)
    try:
      self.ui.refToolBar = QtGui.QToolBar(self)
      sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Fixed)
      sizePolicy.setHorizontalStretch(0)
      sizePolicy.setVerticalStretch(0)
      sizePolicy.setHeightForWidth(self.ui.refToolBar.sizePolicy().hasHeightForWidth())
      self.ui.refToolBar.setSizePolicy(sizePolicy)
      self.ui.refToolBar.setObjectName("refToolBar")
      self.ui.refToolBar.addAction(self.ui.actionReferenceImage)
      self.ui.refToolBar.addAction(self.ui.actionUnloadImage)
      self.ui.refToolBar.addSeparator()
      self.ui.refToolBar.addAction(self.ui.actionZoomIn)
      self.ui.refToolBar.addAction(self.ui.actionZoomOut)
      self.ui.refToolBar.addAction(self.ui.actionPanImage)
      self.ui.refToolBar.addSeparator()
      self.ui.refToolBar.addAction(self.ui.actionImageInfo)
      self.ui.gridLayout_3.addWidget(self.ui.refToolBar, 0, 0, 1, 1)

      self.ui.rawToolBar = QtGui.QToolBar(self)
      sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Fixed)
      sizePolicy.setHorizontalStretch(0)
      sizePolicy.setVerticalStretch(0)
      sizePolicy.setHeightForWidth(self.ui.rawToolBar.sizePolicy().hasHeightForWidth())
      self.ui.rawToolBar.setSizePolicy(sizePolicy)
      self.ui.rawToolBar.setObjectName("rawToolBar")
      self.ui.rawToolBar.addAction(self.ui.actionRawImage)
      self.ui.rawToolBar.addAction(self.ui.actionUnloadImage2)
      self.ui.rawToolBar.addSeparator()
      self.ui.rawToolBar.addAction(self.ui.actionZoomIn2)
      self.ui.rawToolBar.addAction(self.ui.actionZoomOut2)
      self.ui.rawToolBar.addAction(self.ui.actionPanImage2)
      self.ui.rawToolBar.addSeparator()
      self.ui.rawToolBar.addAction(self.ui.actionImageInfo2)
      self.ui.gridLayout_4.addWidget(self.ui.rawToolBar, 0, 0, 1, 1)

      self.autoGcpManager = QgsAutoGCPManager()
      d = QDir(QgsApplication.qgisSettingsDirPath())
      d.mkdir("python")
      d.setPath(d.absolutePath()+"/python")
      d.mkdir("plugins")
      d.setPath(d.absolutePath()+"/plugins")
      d.mkdir("autogcp")
      d.setPath(d.absolutePath()+"/autogcp/")
      self.sqlitePath = QgsApplication.qgisSettingsDirPath() + "python/plugins/autogcp/autogcp.db"

      #A struct for the user settings
      self.settings = AutoGcpSettings()
      try:
        self.xmlHandler = AutoGcpXmlHandler(QgsApplication.qgisSettingsDirPath() + "python/plugins/autogcp/settings.xml")
        self.xmlHandler.setSettings(self.settings)
        self.xmlHandler.openXml()
      except:
        QgsLogger.debug("Could not find the settings file")

      if self.settings.databaseSelection == 1:
        if self.settings.sqliteChoice == 1:
          self.autoGcpManager.connectDatabase(self.settings.sqlitePath)
        else:
          self.autoGcpManager.connectDatabase(self.sqlitePath)
              
      if self.settings.loadGcpTable == 0:
        self.ui.dockGCPWidget.setVisible(False)
      else:
        self.ui.dockGCPWidget.setVisible(True)
      if self.settings.loadRefCanvas == 0:
        self.ui.refDock.setVisible(False)
      else:
        self.ui.refDock.setVisible(True)
      if self.settings.loadRawCanvas == 0:
        self.ui.rawDock.setVisible(False)
      else:
        self.ui.rawDock.setVisible(True)

      self.autoGcpManager.setChipSize(self.settings.chipWidth, self.settings.chipHeight)

      #Raw and ref file paths
      self.refFilePath = ""
      self.rawFilePath = ""

      #Handles the signal when the canvas is clicked
      self.refEmitPoint = QgsMapToolEmitPoint(self.ui.refMapCanvas)
      self.rawEmitPoint = QgsMapToolEmitPoint(self.ui.rawMapCanvas)

      #Handles the signals and slots
      QObject.connect(self.ui.actionAbout, SIGNAL("triggered()"), self.showAbout)
      QObject.connect(self.ui.actionReferenceImage, SIGNAL("triggered()"), self.openFileRef)
      QObject.connect(self.ui.actionRawImage, SIGNAL("triggered()"), self.openFileRaw)
      QObject.connect(self.ui.actionDetectGCP, SIGNAL("triggered()"), self.detectGcps)
      QObject.connect(self.ui.actionCrossReference, SIGNAL("triggered()"), self.matchGcps)
      QObject.connect(self.ui.actionRectfication, SIGNAL("triggered()"), self.showRectificationDialog)
      QObject.connect(self.ui.actionShowTable, SIGNAL("triggered()"), self.showGCPTableByAction)
      QObject.connect(self.ui.dockGCPWidget, SIGNAL("visibilityChanged(bool)"), self.showGCPTableByWidget)
      QObject.connect(self.ui.actionShowRefImage, SIGNAL("triggered()"), self.showRefImageByAction)
      QObject.connect(self.ui.refDock, SIGNAL("visibilityChanged(bool)"), self.showRefImageByWidget)
      QObject.connect(self.ui.actionShowRawImage, SIGNAL("triggered()"), self.showRawImageByAction)
      QObject.connect(self.ui.rawDock, SIGNAL("visibilityChanged(bool)"), self.showRawImageByWidget)
      QObject.connect(self.ui.actionZoomImage, SIGNAL("triggered()"), self.zoomImage)
      QObject.connect(self.ui.actionZoomInAll, SIGNAL("triggered()"), self.zoomInAll)
      QObject.connect(self.ui.actionZoomOutAll, SIGNAL("triggered()"), self.zoomOutAll)
      QObject.connect(self.ui.actionPanAll, SIGNAL("triggered()"), self.panAll)
      QObject.connect(self.ui.actionZoomIn, SIGNAL("triggered()"), self.zoomIn)
      QObject.connect(self.ui.actionZoomOut, SIGNAL("triggered()"), self.zoomOut)
      QObject.connect(self.ui.actionPanImage, SIGNAL("triggered()"), self.pan)
      QObject.connect(self.ui.actionZoomIn2, SIGNAL("triggered()"), self.zoomIn2)
      QObject.connect(self.ui.actionZoomOut2, SIGNAL("triggered()"), self.zoomOut2)
      QObject.connect(self.ui.actionPanImage2, SIGNAL("triggered()"), self.pan2)
      QObject.connect(self.ui.actionImportGCP, SIGNAL("triggered()"), self.importGcps)
      QObject.connect(self.ui.actionExportGCP, SIGNAL("triggered()"), self.exportGcps)
      QObject.connect(self.ui.actionLoadDb, SIGNAL("triggered()"), self.loadDatabase)
      QObject.connect(self.ui.actionSaveDb, SIGNAL("triggered()"), self.saveDatabase)
      QObject.connect(self.ui.actionAddGCP, SIGNAL("triggered()"), self.handleAddGCP)
      QObject.connect(self.ui.actionRemoveGCP, SIGNAL("triggered()"), self.handleRemoveGCP)
      QObject.connect(self.ui.actionMoveGCP, SIGNAL("triggered()"), self.handleMoveGCP)
      QObject.connect(self.ui.actionAdvancedSettings, SIGNAL("triggered()"), self.changeSettings)
      QObject.connect(self.ui.gcpTable, SIGNAL("itemChanged(QTableWidgetItem*) "), self.setGcpVisibility)
      QObject.connect(self.refEmitPoint, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.handleRefGcpDrawing) #Older versions of QGis may not use the "const" eg: canvasClicked(QgsPoint &, Qt::MouseButton)
      QObject.connect(self.rawEmitPoint, SIGNAL("canvasClicked(const QgsPoint &, Qt::MouseButton)"), self.handleRawGcpDrawing) #Older versions of QGis may not use the "const" eg: canvasClicked(QgsPoint &, Qt::MouseButton)
      QObject.connect(self.ui.refMapCanvas, SIGNAL("xyCoordinates(const QgsPoint  &)"), self.updateRefCoordinates)
      QObject.connect(self.ui.rawMapCanvas, SIGNAL("xyCoordinates(const QgsPoint  &)"), self.updateRawCoordinates)
      QObject.connect(self.ui.refMapCanvas, SIGNAL("scaleChanged(double)"), self.updateRefScale)
      QObject.connect(self.ui.rawMapCanvas, SIGNAL("scaleChanged(double)"), self.updateRawScale)
      QObject.connect(self.ui.actionImageInfo, SIGNAL("triggered()"), self.showRefInfo)
      QObject.connect(self.ui.actionImageInfo2, SIGNAL("triggered()"), self.showRawInfo)
      QObject.connect(self.ui.actionUnloadImage, SIGNAL("triggered()"), self.unloadRefImage)
      QObject.connect(self.ui.actionUnloadImage2, SIGNAL("triggered()"), self.unloadRawImage)
      QObject.connect(self.ui.pushButton, SIGNAL("clicked()"), self.deleteAllGcps)
      QObject.connect(self.ui.pushButton_2, SIGNAL("clicked()"), self.deleteSelectedGcps)
      QObject.connect(self.ui.gcpTable, SIGNAL("itemSelectionChanged()"), self.highlightGcp)
      QObject.connect(self.ui.refMapCanvas, SIGNAL("extentsChanged()"), self.setRawExtents)
      QObject.connect(self.ui.rawMapCanvas, SIGNAL("extentsChanged()"), self.setRefExtents)

      #Set the GCP Table visibility to true
      self.ui.actionShowTable.setChecked(True)

      #View actions - pan and zoom
      self.refToolPan = QgsMapToolPan(self.ui.refMapCanvas)
      self.refToolPan.setAction(self.ui.actionPanImage)
      self.refToolZoomIn = QgsMapToolZoom(self.ui.refMapCanvas, False) # false = in
      self.refToolZoomIn.setAction(self.ui.actionZoomIn)
      self.refToolZoomOut = QgsMapToolZoom(self.ui.refMapCanvas, True) # true = out
      self.refToolZoomOut.setAction(self.ui.actionZoomOut)

      self.rawToolPan = QgsMapToolPan(self.ui.rawMapCanvas)
      self.rawToolPan.setAction(self.ui.actionPanImage2)
      self.rawToolZoomIn = QgsMapToolZoom(self.ui.rawMapCanvas, False) # false = in
      self.rawToolZoomIn.setAction(self.ui.actionZoomIn2)
      self.rawToolZoomOut = QgsMapToolZoom(self.ui.rawMapCanvas, True) # true = out
      self.rawToolZoomOut.setAction(self.ui.actionZoomOut)

      #Items for the GCP table
      self.gcpItems = []

      #Gcp highlighters
      self.gcpHighlights = []

      #Projection structs
      self.refProjectionSet = False
      self.rawProjectionSet = False

      self.ui.refMapCanvas.setCanvasColor(QColor(self.settings.refCanvasColor))
      self.ui.refMapCanvas.refresh()
      self.ui.rawMapCanvas.setCanvasColor(QColor(self.settings.rawCanvasColor))
      self.ui.rawMapCanvas.refresh()

      self.unloadRefImage()
      self.unloadRawImage()

      #This variable is required to ensure that the canvas extents are not set in an infinte loop
      self.extentsSet = 0

      #Needed to filter images for the default value of the database option (loading)
      self.imageFilter = 0
         
      width = self.ui.refMapCanvas.size().width()
      height = self.ui.refMapCanvas.size().height()
      self.ui.refMapCanvas.map().resize( QSize( width, height ) )
            
      width = self.ui.rawMapCanvas.size().width()
      height = self.ui.rawMapCanvas.size().height()
      self.ui.rawMapCanvas.map().resize( QSize( width, height ) )

    except:
      QgsLogger.debug( "Setting up the main window failed: ", 1, "gui.py", "MainWindow::__init__()")

    #FoxHat Added - Start

  #Highlight GCP markers on the canvas that were selected in the table
  def highlightGcp(self):
        try:
          #Remove all the highlight markers
          for i in range(0, len(self.gcpHighlights)):
            self.ui.refMapCanvas.scene().removeItem(self.gcpHighlights[i].getRefMarker())
            self.ui.rawMapCanvas.scene().removeItem(self.gcpHighlights[i].getRawMarker())
          self.gcpHighlights = list()
          #Get a set of all GCPs that were selected
          theSet = set()
          selected = self.ui.gcpTable.selectedIndexes()
          for i in range(0, len(selected)):
            theSet.add(selected[i].row())
          #Draw the new higlights
          for i in range(0, len(theSet)):
            index = theSet.pop()
            gcp = AutoGcpItem(self.ui.refMapCanvas, self.ui.rawMapCanvas)
            if self.gcpItems[index].getRefMarker().isVisible():
              gcp.setRefCenter(self.gcpItems[index].getRefCenter())
              gcp.getRefMarker().setIcon(self.settings.selectedIconType, self.settings.selectedIconColor)
              gcp.getRefMarker().show()
            if self.gcpItems[index].hasRaw() and self.gcpItems[index].getRawMarker().isVisible():
              gcp.setRawCenter(self.gcpItems[index].getRawCenter())
              gcp.getRawMarker().setIcon(self.settings.selectedIconType, self.settings.selectedIconColor)
              gcp.getRawMarker().show()
            self.gcpHighlights.append(gcp)
        except:
          QgsLogger.debug( "Highlighting GCP failed", 1, "guibase.py", "Ui_MainWindow::highlightGcp()")

  #Remove all GCPS from the canvas, table and the AutoGcp manager
  def deleteAllGcps(self):
        try:
          self.showStatus("Removing all GCPs", 3)
          #Remove all the highlight markers
          for i in range(0, len(self.gcpHighlights)):
            self.ui.refMapCanvas.scene().removeItem(self.gcpHighlights[i].getRefMarker())
            self.ui.rawMapCanvas.scene().removeItem(self.gcpHighlights[i].getRawMarker())
          #Remove all GCP markers
          for i in range(0, len(self.gcpItems)):
            self.ui.refMapCanvas.scene().removeItem(self.gcpItems[i].getRefMarker())
            self.ui.rawMapCanvas.scene().removeItem(self.gcpItems[i].getRawMarker())
          #Remove all GCPs from the AutoGcp manager
          hasGcps = False
          try:
            if self.autoGcpManager.gcpSet().size() > 0:
              hasGcps = True
          except:
            QgsLogger.debug( "No GCPs to delete", 1, "guibase.py", "Ui_MainWindow::deleteAllGcps()")
          self.autoGcpManager.clearGcpSet()
          #Update the GCP table, hence all entries will be removed
          self.gcpHighlights = list()
          self.gcpItems = list()
          self.updateGcpTable()
          if hasGcps:
            self.showStatus("All GCPs were removed", 3)
        except:
          QgsLogger.debug( "Deleting all GCPs failed", 1, "guibase.py", "Ui_MainWindow::deleteAllGcps()")

  #Removes the GCPs that were previously selected from the canvas, table and the AutoGcp manager
  def deleteSelectedGcps(self):
        try:
          self.showStatus("Removing selected GCPs", 3)
          #Get a set of all GCPs that were selected
          theSet = set()
          selected = self.ui.gcpTable.selectedIndexes()
          for i in range(0, len(selected)):
            theSet.add(selected[i].row())
          #Remove GCPs one by one
          while len(theSet) > 0:
            #Get the GCP index from the table according to the ID
            index = -1
            id = theSet.pop()
            for i in range(0, len(self.gcpItems)):
              #The ID is one larger than the index in the table
              if self.gcpItems[i].getId() == (id+1):
                index = i
                break
            self.deleteGcp(index)
          #Update the GCP table, hence all entries will be removed
          self.updateGcpTable()
          self.showStatus("Selected GCPs were removed", 3)
        except:
          QgsLogger.debug( "Deleting selected GCPs failed", 1, "guibase.py", "Ui_MainWindow::deleteSelectedGcps()")

  #Removes a single GCP from the canvas, table and the AutoGcp manager
  def deleteGcp(self, index):
        try:
          self.showStatus("Removing GCP", 3)
          #Remove all the highlight markers
          for i in range(0, len(self.gcpHighlights)):
            self.ui.refMapCanvas.scene().removeItem(self.gcpHighlights[i].getRefMarker())
            self.ui.rawMapCanvas.scene().removeItem(self.gcpHighlights[i].getRawMarker())
          self.gcpHighlights = list()
          gcpItem = self.gcpItems.pop(index)
          self.ui.refMapCanvas.scene().removeItem(gcpItem.getRefMarker())
          self.ui.rawMapCanvas.scene().removeItem(gcpItem.getRawMarker())
          gcp = QgsGcp()
          gcp.setRefX(gcpItem.getRefCenter().x())
          gcp.setRefY(gcpItem.getRefCenter().y())
          gcp.setRawX(gcpItem.getRawCenter().x())
          gcp.setRawY(gcpItem.getRawCenter().y())
          self.autoGcpManager.removeGcp(gcp)
          self.showStatus("GCP was removed", 3)
        except:
          QgsLogger.debug( "Deleting GCP failed", 1, "guibase.py", "Ui_MainWindow::deleteGcp()")

  #Displays a message in the status bar for "time" number of seconds
  def showStatus(self, msg, time = 0):
        try:
          self.ui.statusBar.showMessage(msg, (time*1000))
        except:
          QgsLogger.debug( "Updating status failed", 1, "guibase.py", "Ui_MainWindow::showStatus()")

  #Removes the reference image from the canvas and deletes all GCPs
  def unloadRefImage(self):
        try:
          self.deleteAllGcps()
          layerSet = []
          self.ui.refMapCanvas.enableAntiAliasing(True)
          self.ui.refMapCanvas.freeze(False)
          self.ui.refMapCanvas.setLayerSet(layerSet)
          self.ui.refMapCanvas.setVisible(True)
          self.ui.refMapCanvas.clearExtentHistory()
          self.ui.refMapCanvas.refresh()
          self.refFilePath = ""
          self.showStatus("Reference image unloaded", 3)
        except:
          QgsLogger.debug( "Unloading reference image failed", 1, "guibase.py", "Ui_MainWindow::unloadRefImage()")

  #Removes the raw image from the canvas and deletes all GCPs
  def unloadRawImage(self):
        try:
          self.deleteAllGcps()
          layerSet = []
          self.ui.rawMapCanvas.enableAntiAliasing(True)
          self.ui.rawMapCanvas.freeze(False)
          self.ui.rawMapCanvas.setLayerSet(layerSet)
          self.ui.rawMapCanvas.setVisible(True)
          self.ui.rawMapCanvas.clearExtentHistory()
          self.ui.rawMapCanvas.refresh()
          self.rawFilePath = ""
          self.showStatus("Raw image unloaded", 3)
        except:
          QgsLogger.debug( "Unloading raw image failed", 1, "guibase.py", "Ui_MainWindow::unloadRawImage()")

  #Displays the properties of a certain image
  def showImageInfo(self, path, layer, canvas):
        try:
          self.imageDialog = ImageDialog(self)
          self.imageDialog.setImageInfo(self.autoGcpManager.imageInfo(path))
          self.imageDialog.setComponents(layer, canvas)
          if path == self.refFilePath:
            self.imageDialog.setProjection(self.autoGcpManager, self.autoGcpManager.referenceImage(), self.autoGcpManager.getProjectionSrsID(0), self.autoGcpManager.getProjectionEpsg(0))
          elif path == self.rawFilePath:
            self.imageDialog.setProjection(self.autoGcpManager, self.autoGcpManager.rawImage(), self.autoGcpManager.getProjectionSrsID(1), self.autoGcpManager.getProjectionEpsg(1))
          self.imageDialog.show()
        except:
          QgsLogger.debug( "Showing image information dialog failed", 1, "guibase.py", "Ui_MainWindow::showImageInfo()")

  #Displays the properties of the reference image
  def showRefInfo(self):
        try:
          if len(self.refFilePath) > 0:
            self.showImageInfo(self.refFilePath, self.refLayer, self.ui.refMapCanvas)
        except:
          QgsLogger.debug( "Showing reference image information failed", 1, "guibase.py", "Ui_MainWindow::showRefInfo()")

  #Displays the properties of the raw image
  def showRawInfo(self):
        try:
          if len(self.rawFilePath) > 0:
            self.showImageInfo(self.rawFilePath, self.rawLayer, self.ui.rawMapCanvas)
        except:
          QgsLogger.debug( "Showing raw image information failed", 1, "guibase.py", "Ui_MainWindow::showRawInfo()")

  #Displays the coordinates of the mouse pointer on the reference canvas in the reference status bar
  def updateRefCoordinates(self, point):
        try:
          xCoordinate = point.x()
          yCoordinate = point.y()
          if xCoordinate == 0.0 and yCoordinate == 0.0:
            x = "0000000.0000"
            y = "0000000.0000"
          else:
            x = "%.4f" % xCoordinate
            y = "%.4f" % yCoordinate
          self.ui.refCoordinateText.setText(x+"  "+y)
        except:
          QgsLogger.debug( "Updating the reference coordinates failed", 1, "guibase.py", "Ui_MainWindow::updateRefCoordinates()")

  #Displays the coordinates of the mouse pointer on the raw canvas in the raw status bar
  def updateRawCoordinates(self, point):
        try:
          xCoordinate = point.x()
          yCoordinate = point.y()
          if xCoordinate == 0.0 and yCoordinate == 0.0:
            x = "0000000.0000"
            y = "0000000.0000"
          else:
            x = "%.4f" % xCoordinate
            y = "%.4f" % yCoordinate
          self.ui.rawCoordinateText.setText(x+"  "+y)
        except:
          QgsLogger.debug( "Updating the raw coordinates failed", 1, "guibase.py", "Ui_MainWindow::updateRawCoordinates()")

  #Displays the scale of the reference image in the reference status bar
  def updateRefScale(self, scale):
        try:
          if scale >= 1.0:
            self.ui.refScaleText.setText("1:" + QString.number(scale, 'f', 0 ))
          elif scale > 0.0:
            self.ui.refScaleText.setText(QString.number(1.0/scale, 'f', 0) + ":1")
          else:
            self.ui.refScaleText.setText("Invalid scale")
        except:
          QgsLogger.debug( "Updating the reference scale failed", 1, "guibase.py", "Ui_MainWindow::updateRefScale()")

  #Displays the scale of the raw image in the raw status bar
  def updateRawScale(self, scale):
        try:
          if scale >= 1.0:
            self.ui.rawScaleText.setText("1:" + QString.number(scale, 'f', 0 ))
          elif scale > 0.0:
            self.ui.rawScaleText.setText(QString.number(1.0/scale, 'f', 0) + ":1")
          else:
            self.ui.rawScaleText.setText("Invalid scale")
        except:
          QgsLogger.debug( "Updating the raw scale failed", 1, "guibase.py", "Ui_MainWindow::updateRawScale()")

  #Displays a message to the user
  def showMessage(self, string, type = "info"):
        try:
          msgbox = QtGui.QMessageBox(self)
          msgbox.setText(string)
          msgbox.setWindowTitle("AutoGCP Message")
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
          ret = msgbox.exec_()
        except:
          QgsLogger.debug( "Showing message box failed", 1, "guibase.py", "Ui_MainWindow::showMessage()")

  #Displays the About dialog of the plugin
  def showAbout(self):
        try:
          self.aboutDialog = AboutDialog(self)
          self.aboutDialog.exec_()
        except:
          QgsLogger.debug( "Showing about dialog failed", 1, "guibase.py", "Ui_MainWindow::showAbout()")

  def searchDatabase(self):
        try:
          self.showStatus("Searching database for GCPs", 2)
          return self.autoGcpManager.loadDatabase(self.imageFilter)
        except:
          QgsLogger.debug( "Searching database by image failed", 1, "guibase.py", "Ui_MainWindow::searchDatabase()")

  def searchDatabaseByHash(self, hash):
        try:
          self.showStatus("Searching database for GCPs", 2)
          return self.autoGcpManager.loadDatabaseByHash(hash)
        except:
          QgsLogger.debug( "Searching database by hash failed", 1, "guibase.py", "Ui_MainWindow::searchDatabaseByHash()")

  def searchDatabaseByLocation(self, pWX, pWY, pHX, pHY, tLX, tLY, proj):
        try:
          self.showStatus("Searching database for GCPs", 2)
          return self.autoGcpManager.loadDatabaseByLocation(pWX, pWY, pHX, pHY, tLX, tLY, proj)
        except:
          QgsLogger.debug( "Searching database by location failed", 1, "guibase.py", "Ui_MainWindow::searchDatabaseByLocation()")

  def loadGcps(self, canvas, option):
        try:
          if (option > -1):
            display = True
            gcpsDetected = True
            try:
              if canvas == 0 and option < 1:
                gcpsDetected = self.autoGcpManager.detectGcps(self.imageFilter)
              elif option < 1:
                gcpsDetected = self.autoGcpManager.detectGcps(self.imageFilter)
              if (self.settings.gcpLoading or option > 0) and not gcpsDetected:
                self.showMessage("No GCPs were found in the database", "warning")
                self.showStatus("No GCPs were found in the database", 3)
              elif self.settings.gcpLoading == 1 and gcpsDetected:
                msgBox = QMessageBox()
                msgBox.setText("Some GCPs where detected in the database.")
                msgBox.setInformativeText("Do you want to load them?")
                msgBox.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
                msgBox.setDefaultButton(QMessageBox.Yes)
                msgBox.setIcon(QMessageBox.Information)
                result = msgBox.exec_()
                if result == QMessageBox.Yes:
                  display = True
                elif result == QMessageBox.No:
                  display = False
                  self.deleteAllGcps()
                  self.showMessage("No GCPs could be retrieved from the database", "warning")
                  self.showStatus("No GCPs could be retrieved from the database", 3)
            except:
              self.showMessage("No GCPs could be retrieved from the database", "warning")
              self.showStatus("No GCPs could be retrieved from the database", 3)
            if gcpsDetected and display:
              self.deleteAllGcps()
              gcpSet = QgsGcpSet()
              if (self.settings.gcpLoading == 0 or self.settings.gcpLoading == 1) and option == -1:
                gcpSet = self.searchDatabase()
              elif option == 0:
                gcpSet = self.searchDatabase()
              elif option == 1:
                gcpSet = self.searchDatabaseByHash(self.imageHash)
              try:
                gcpList = gcpSet.constList()
                if canvas == 0:
                  for i in range(0, gcpSet.size()):
                    gcpPoint = QgsPoint(gcpList[i].refX(), gcpList[i].refY())
                    gcpItem = AutoGcpItem(self.ui.refMapCanvas, self.ui.rawMapCanvas)
                    gcpItem.setMatch(-1)
                    self.gcpItems.append(gcpItem)
                    self.drawGcp(gcpItem, gcpPoint, 0)
                else:
                  for i in range(0, len(self.gcpHighlights)):
                    self.ui.refMapCanvas.scene().removeItem(self.gcpHighlights[i].getRefMarker())
                    self.ui.rawMapCanvas.scene().removeItem(self.gcpHighlights[i].getRawMarker())
                  for i in range(0, len(self.gcpItems)):
                    self.ui.refMapCanvas.scene().removeItem(self.gcpItems[i].getRefMarker())
                    self.ui.rawMapCanvas.scene().removeItem(self.gcpItems[i].getRawMarker())
                  for i in range(0, len(gcpList)):
                    gcpItem = AutoGcpItem(self.ui.refMapCanvas, self.ui.rawMapCanvas)
                    gcpItem.setMatch(-1)
                    self.gcpItems.append(gcpItem)
                    gcpPoint = QgsPoint(gcpList[i].refX(), gcpList[i].refY())
                    self.drawGcp(gcpItem, gcpPoint, 0)
                    gcpPoint = QgsPoint(gcpList[i].rawX(), gcpList[i].rawY())
                    self.drawGcp(gcpItem, gcpPoint, 1)
                self.updateGcpTable()
                self.ui.refMapCanvas.refresh()
                self.ui.rawMapCanvas.refresh()
                self.showStatus("GCPs were loaded from the database", 3)
              except:
                self.showMessage("No GCPs could be retrieved from the database", "warning")
                self.showStatus("No GCPs could be retrieved from the database", 3)
        except:
          QgsLogger.debug( "Loading GCPs failed", 1, "guibase.py", "Ui_MainWindow::loadGcps()")


  def openFile(self, title, defaultPath, refRaw):
        try:
          filter = ""
          QgsRasterLayer.buildSupportedRasterFileFilter( filter );
          #files = QFileDialog.getOpenFileNames(None, title, defaultPath, filter)
          dialog = QFileDialog(None, title, defaultPath, filter)
          dialog.setFileMode(QFileDialog.ExistingFile)
          if dialog.exec_() == QDialog.Accepted:
	    files = dialog.selectedFiles()
         
          if len(files) > 0:
            qApp.processEvents()
            tempPath = files[0]
            QgsLogger.debug(tempPath)
            if refRaw == 0:
              self.settings.lastOpenRefPath = tempPath
            elif refRaw == 1:
              self.settings.lastOpenRawPath = tempPath
            try:
              self.xmlHandler.setSettings(self.settings)
              self.xmlHandler.saveXml()
            except:
              QgsLogger.debug("Could not save the last opened path")
            return tempPath
          else:
            QgsLogger.debug("No file selected")
            return ""
        except:
          QgsLogger.debug( "Handling open file dialog failed: ", 1, "guibase.py", "Ui_MainWindow::openFile()")

  def openFileRef(self):
        try:
          self.deleteAllGcps()
          if self.settings.refPathChoice == 0:
            tempPath = self.openFile("Open Reference Image", "", 0)
          elif self.settings.refPathChoice == 1:
            tempPath = self.openFile("Open Reference Image", self.settings.lastOpenRefPath, 0)
          elif self.settings.refPathChoice == 2:
            tempPath = self.openFile("Open Reference Image", self.settings.refPath, 0)
          if len(tempPath) > 0:
            self.showStatus("Loading reference image")
            self.refFilePath = tempPath
            app = QCoreApplication.instance()
            app.processEvents()
            fileInfo = QFileInfo(self.refFilePath)

            self.refLayer = QgsRasterLayer(self.refFilePath, fileInfo.completeBaseName())
            mapRegistry = QgsMapLayerRegistry.instance()
            mapRegistry.addMapLayer(self.refLayer, False)
            layerSet = []
            mapCanvasLayer = QgsMapCanvasLayer(self.refLayer, True)
            layerSet.append(mapCanvasLayer)
            self.ui.refMapCanvas.setExtent(self.refLayer.extent())
            self.ui.refMapCanvas.enableAntiAliasing(True)
            self.ui.refMapCanvas.freeze(False)
            self.ui.refMapCanvas.setLayerSet(layerSet)
            width = self.ui.refMapCanvas.size().width();
	    height = self.ui.refMapCanvas.size().height();
            self.ui.refMapCanvas.map().resize( QSize( width, height ) );
	    self.ui.refMapCanvas.updateCanvasItemPositions();
	    self.ui.refMapCanvas.updateScale();
            self.ui.refMapCanvas.setVisible(True)
            self.ui.refMapCanvas.refresh()
            progressDialog = ProgressDialog(self)
            progressDialog.setText("Opening reference image...")
            progressDialog.setManager(self.autoGcpManager)
            progressDialog.show()
            con = self.autoGcpManager.executeOpenReferenceImage(self.refFilePath)
            progressDialog.progress()
	    progressDialog.close()
	    if(not con):
              if self.autoGcpManager.lastError() == 1:
                self.showMessage("The file could not be found")
              elif self.autoGcpManager.lastError() == 2:
                self.showMessage("The file could not be opened")
              elif self.autoGcpManager.lastError() == 3:
                self.refProjectionDialog = ProjectionDialog(self)
                QObject.connect(self.refProjectionDialog, SIGNAL("accepted()"), self.acceptedRefProjection)
                self.refProjectionDialog.show()
            else:
              self.refProjectionSet = True
            self.bandCount = copy.copy(self.autoGcpManager.imageInfo(self.refFilePath).pRasterBands)
            if len(self.rawFilePath) > 0:
              self.imageFilter = 2
            else:
              self.imageFilter = 0
            self.loadGcps(0, -1)
            self.ui.refMapCanvas.mapRenderer().setDestinationSrs(QgsCoordinateReferenceSystem(self.autoGcpManager.getProjectionEpsg(0)))
        except:
          QgsLogger.debug( "Opening referenece image failed: ", 1, "guibase.py", "Ui_MainWindow::openFileRef()")

  def acceptedRefProjection(self):
        try:
          self.refProjectionSet = True
          self.autoGcpManager.setProjection(self.autoGcpManager.referenceImage(), self.refProjectionDialog.getProjectionString())
          self.ui.refMapCanvas.mapRenderer().setDestinationSrs(QgsCoordinateReferenceSystem(self.autoGcpManager.getProjectionEpsg(0)))
        except:
          QgsLogger.debug( "Accepting referenece image projection failed", 1, "guibase.py", "Ui_MainWindow::acceptedRefProjection()")

  def openFileRaw(self):
        try:
          if self.settings.rawPathChoice == 0:
            tempPath = self.openFile("Open Raw Image", "", 1)
          elif self.settings.rawPathChoice == 1:
            tempPath = self.openFile("Open Raw Image", self.settings.lastOpenRawPath, 1)
          elif self.settings.rawPathChoice == 2:
            tempPath = self.openFile("Open Raw Image", self.settings.rawPath, 1)
          if len(tempPath) > 0:
            self.showStatus("Loading raw image")
            self.rawFilePath = tempPath
            app = QCoreApplication.instance()
            app.processEvents()
            fileInfo = QFileInfo(self.rawFilePath)
            self.rawLayer = QgsRasterLayer(self.rawFilePath, fileInfo.completeBaseName())
            mapRegistry = QgsMapLayerRegistry.instance()
            mapRegistry.addMapLayer(self.rawLayer, False)
            layerSet = []
            mapCanvasLayer = QgsMapCanvasLayer(self.rawLayer, True)
            layerSet.append(mapCanvasLayer)
            self.ui.rawMapCanvas.setExtent(self.rawLayer.extent())
            self.ui.rawMapCanvas.enableAntiAliasing(True)
            self.ui.rawMapCanvas.freeze(False)
            self.ui.rawMapCanvas.setLayerSet(layerSet)
            width = self.ui.rawMapCanvas.size().width();
	    height = self.ui.rawMapCanvas.size().height();
            self.ui.rawMapCanvas.map().resize( QSize( width, height ) );
            self.ui.rawMapCanvas.updateCanvasItemPositions();
	    self.ui.rawMapCanvas.updateScale();
            self.ui.rawMapCanvas.setVisible(True)
            self.ui.rawMapCanvas.refresh()
            progressDialog = ProgressDialog(self)
            progressDialog.setText("Opening raw image...")
            progressDialog.setManager(self.autoGcpManager)
            progressDialog.show()
            con = self.autoGcpManager.executeOpenRawImage(self.rawFilePath)
	    progressDialog.progress()
	    progressDialog.close()
            if(not con):
              if self.autoGcpManager.lastError() == 1:
                self.showMessage("The file could not be found")
              elif self.autoGcpManager.lastError() == 2:
                self.showMessage("The file could not be opened")
              elif self.autoGcpManager.lastError() == 3:
                self.rawProjectionDialog = ProjectionDialog(self)
                QObject.connect(self.rawProjectionDialog, SIGNAL("accepted()"), self.acceptedRawProjection)
                self.rawProjectionDialog.show()
            else:
              self.rawProjectionSet = True
            if len(self.refFilePath) > 0:
              self.imageFilter = 2
            else:
              self.imageFilter = 1
            self.loadGcps(1, -1)
            self.ui.rawMapCanvas.mapRenderer().setDestinationSrs(QgsCoordinateReferenceSystem(self.autoGcpManager.getProjectionEpsg(1)))
        except:
          QgsLogger.debug( "Opening raw image failed", 1, "guibase.py", "Ui_MainWindow::openFileRaw()")

  def acceptedRawProjection(self):
        try:
          self.rawProjectionSet = True
          self.autoGcpManager.setProjection(self.autoGcpManager.rawImage(), self.rawProjectionDialog.getProjectionString())
          self.ui.rawMapCanvas.mapRenderer().setDestinationSrs(QgsCoordinateReferenceSystem(self.autoGcpManager.getProjectionEpsg(1)))
        except:
          QgsLogger.debug( "Accepting raw image projection failed", 1, "guibase.py", "Ui_MainWindow::acceptedRawProjectio()")

  def detectGcps(self):
        try:
          self.deleteAllGcps()
          if len(self.refFilePath) == 0:
            self.showMessage("Please open a reference image", "info")
          else:
            self.showStatus("Detecting GCPs")
            self.autoGcpManager.setGcpCount(self.settings.gcpAmount)
            progressDialog = ProgressDialog(self)
            progressDialog.setText("Detecting GCPs...")
            progressDialog.setManager(self.autoGcpManager)
            progressDialog.show()
            self.autoGcpManager.executeExtraction()
            time.sleep(0.1)
            progressDialog.progress()
            progressDialog.close()
            gcpPoints = self.autoGcpManager.gcpSet()
            gcpList = gcpPoints.constList()
            for i in range(0, gcpPoints.size()):
              gcpPoint = QgsPoint(gcpList[i].refX(), gcpList[i].refY())
              gcpItem = AutoGcpItem(self.ui.refMapCanvas, self.ui.rawMapCanvas)
              gcpItem.setMatch(gcpList[i].correlationCoefficient()*100)
              self.gcpItems.append(gcpItem)
              self.drawGcp(gcpItem, gcpPoint, 0)
            self.updateGcpTable()
            self.showStatus(str(self.settings.gcpAmount)+" GCPs detected", 3)
            self.ui.refMapCanvas.refresh()
            self.ui.rawMapCanvas.refresh()
        except:
          QgsLogger.debug( "Detecting GCPs failed", 1, "guibase.py", "Ui_MainWindow::detectGcps()")

  def matchGcps(self):
        try:
          self.clearOldInfo()
          if len(self.rawFilePath) == 0:
            self.showMessage("Please open a raw image", "info")
          else:
            self.showStatus("Cross-referencing GCPs")
            progressDialog = ProgressDialog(self)
            progressDialog.setText("Cross-referencing GCPs...")
            progressDialog.setManager(self.autoGcpManager)
            progressDialog.show()
            self.autoGcpManager.setCorrelationThreshold(self.settings.correlationThreshold)
            self.autoGcpManager.executeCrossReference()
            progressDialog.progress()
            progressDialog.close()
            gcpPoints = self.autoGcpManager.gcpSet()
            gcpList = gcpPoints.constList()
            for i in range(0, gcpPoints.size()):
              gcpPoint = QgsPoint(gcpList[i].refX(), gcpList[i].refY())
              gcpPoint2 = QgsPoint(gcpList[i].rawX(), gcpList[i].rawY())
              gcpItem = AutoGcpItem(self.ui.refMapCanvas, self.ui.rawMapCanvas)
              gcpItem.setMatch(gcpList[i].correlationCoefficient()*100)
              self.gcpItems.append(gcpItem)
              self.drawGcp(gcpItem, gcpPoint, 0)
              self.drawGcp(gcpItem, gcpPoint2, 1)
            self.updateGcpTable()
            self.showStatus("GCPs cross-referenced", 3)
            self.ui.refMapCanvas.refresh()
            self.ui.rawMapCanvas.refresh()
        except:
          QgsLogger.debug( "Cross-referencing GCPs failed", 1, "guibase.py", "Ui_MainWindow::matchGcps()")

  def showRectificationDialog(self):
        try:
          """if len(self.rawFilePath) == 0:
            self.showMessage("Please open a raw image and detect and cross-reference the GCPs", "info")
          elif self.autoGcpManager.gcpSet() == None:
	    self.showMessage("Please detect and cross-reference the GCPs", "info")
          elif self.autoGcpManager.gcpSet().size() < 1:
            self.showMessage("Please detect and cross-reference the GCPs", "info")
          else:"""
          self.rectificationDialog = RectificationDialog(self, self.ui.refMapCanvas)
          self.rectificationPath = copy.copy(self.settings.lastOpenRawPath)
          self.rectificationDialog.setInformation(self.rectificationPath)
          self.rectificationDialog.setDriverInfo(self.autoGcpManager.readDriverSource(QgsApplication.pkgDataPath() + "/python/plugins/autogcp/drivers/outputdrivers.db"))
          QObject.connect(self.rectificationDialog, SIGNAL("accepted()"), self.rectify)
          self.rectificationDialog.exec_()
        except:
          QgsLogger.debug( "Showing rectification dialog failed: ", 1, "guibase.py", "Ui_MainWindow::showRectificationDialog()")

  def rectify(self):
        try:
          self.rectificationPath = self.rectificationDialog.getOutputPath()
          self.rectificationAlgorithm = self.rectificationDialog.getAlgorithm()
          self.showStatus("Starting rectification")
          self.autoGcpManager.setDestinationPath(self.rectificationPath)
          self.autoGcpManager.setOutputDriver(self.rectificationDialog.getDriverName())
          progressDialog = ProgressDialog(self)
          progressDialog.setText("Rectifying image...")
          progressDialog.setManager(self.autoGcpManager)
          progressDialog.show()
          if self.rectificationAlgorithm == 0:
            self.autoGcpManager.executeGeoTransform()            
          elif self.rectificationAlgorithm == 1:
            self.autoGcpManager.setRpcThreshold(self.settings.rmsError)
            self.autoGcpManager.executeOrthorectification()            
          elif self.rectificationAlgorithm == 2:
            self.autoGcpManager.executeGeoreference()
          progressDialog.progress()
          progressDialog.close()
          if self.autoGcpManager.status() == 0:
            self.showMessage("The raw image has been rectified.\n\nOuput file:\n"+self.rectificationPath, "info")
          else:
	    imagesAreTheSame = True
	    gcpPoints = self.autoGcpManager.gcpSet()
            gcpList = gcpPoints.constList()
            for i in range(0, gcpPoints.size()):
	      if gcpList[i].correlationCoefficient() != 1:
		imagesAreTheSame = False
		break
	    if imagesAreTheSame:
	      self.showMessage("The raw image could not be rectified.\nAll GCPs have a match quality of 100%,\nthe reference and raw images are probably the same.", "warning")
	    else:
	      self.showMessage("The raw image could not be rectified.\n\nSuggested output file:\n"+self.rectificationPath, "warning")
        except:
          QgsLogger.debug( "Rectification failed: ", 1, "guibase.py", "Ui_MainWindow::rectify()")

  def showGCPTableByAction(self):
        try:
          if self.ui.actionShowTable.isChecked():
            self.dockGCPWidget.setVisible(True)
          else:
            self.dockGCPWidget.setVisible(False)
        except:
          QgsLogger.debug( "Showing GCP table by action failed", 1, "guibase.py", "Ui_MainWindow::showGCPTableByAction()")

  def showGCPTableByWidget(self):
        try:
          if self.dockGCPWidget.isVisible():
            self.ui.actionShowTable.setChecked(True)
          else:
            self.ui.actionShowTable.setChecked(False)
        except:
          QgsLogger.debug( "Showing GCP table by widget failed", 1, "guibase.py", "Ui_MainWindow::showGCPTableByWidget()")

  def showRefImageByAction(self):
        try:
          if self.ui.actionZoomOutShowRefImage.isChecked():
            self.refDock.setVisible(True)
          else:
            self.refDock.setVisible(False)
        except:
          QgsLogger.debug( "Showing reference image by action failed", 1, "guibase.py", "Ui_MainWindow::showRefImageByAction()")

  def showRefImageByWidget(self):
        try:
          if self.refDock.isVisible():
            self.ui.actionZoomOutShowRefImage.setChecked(True)
          else:
            self.ui.actionZoomOutShowRefImage.setChecked(False)
        except:
          QgsLogger.debug( "Showing reference image by widget failed", 1, "guibase.py", "Ui_MainWindow::showRefImageByWidget()")

  def showRawImageByAction(self):
        try:
          if self.ui.actionZoomOutShowRawImage.isChecked():
            self.rawDock.setVisible(True)
          else:
            self.rawDock.setVisible(False)
        except:
          QgsLogger.debug( "Showing raw image by action failed", 1, "guibase.py", "Ui_MainWindow::showRawImageByAction()")

  def showRawImageByWidget(self):
        try:
          if self.rawDock.isVisible():
            self.ui.actionZoomOutShowRawImage.setChecked(True)
          else:
            self.ui.actionZoomOutShowRawImage.setChecked(False)
        except:
          QgsLogger.debug( "Showing raw image by widget failed", 1, "guibase.py", "Ui_MainWindow::showRawImageByWidget()")

  def handleRefGcpDrawing(self, point, button):
        try:
          if self.ui.actionAddGCP.isChecked():
            gcpItem = AutoGcpItem(self.ui.refMapCanvas, self.ui.rawMapCanvas)
            gcpItem.setMatch(-1)
            self.gcpItems.append(gcpItem)
            self.drawGcp(gcpItem, point, 0)
            self.updateGcpTable()
            self.ui.refMapCanvas.unsetMapTool(self.refEmitPoint)
            self.ui.rawMapCanvas.setMapTool(self.rawEmitPoint)
            self.ui.actionZoomOut.setChecked(False)
            self.ui.actionZoomIn.setChecked(False)
            self.ui.actionPanImage.setChecked(False)
            self.ui.actionRemoveGCP.setChecked(False)
            self.ui.actionMoveGCP.setChecked(False)
            self.manualGcp = QgsGcp()
            self.manualGcp.setRefX(point.x())
            self.manualGcp.setRefY(point.y())
            self.autoGcpManager.addGcp(self.manualGcp)
            self.showStatus("GCP added", 3)
          elif self.ui.actionRemoveGCP.isChecked():
            for i in range(0, len(self.gcpItems)):
              refCenter = self.gcpItems[i].getRefCanvasPoint()
              refPoint = QgsVertexMarker(self.ui.refMapCanvas).toCanvasCoordinates(point)
              if (refPoint.x() >= refCenter.x()) and (refPoint.x() < (refCenter.x()+22)) and (refPoint.y() <= refCenter.y()) and (refPoint.y() > (refCenter.y()-28)):
                self.deleteGcp(i)
                self.updateGcpTable()
                break
          elif self.ui.actionMoveGCP.isChecked():
            if self.refMoved == -1:
              for i in range(0, len(self.gcpItems)):
                refCenter = self.gcpItems[i].getRefCanvasPoint()
                refPoint = QgsVertexMarker(self.ui.refMapCanvas).toCanvasCoordinates(point)
                if (refPoint.x() >= refCenter.x()) and (refPoint.x() < (refCenter.x()+22)) and (refPoint.y() <= refCenter.y()) and (refPoint.y() > (refCenter.y()-28)):
                  self.ui.refMapCanvas.scene().removeItem(self.gcpItems[i].getRefMarker())
                  self.updateGcpTable()
                  self.refMoved = i
                  self.movedGcp = self.autoGcpManager.gcpSet().constList()[i]
                  break
            else:
                gcpItem = AutoGcpItem(self.ui.refMapCanvas, self.ui.rawMapCanvas)
                gcpItem.setMatch(-1)
                self.manualGcp = QgsGcp()
                self.manualGcp.setRefX(point.x())
                self.manualGcp.setRefY(point.y())
                self.manualGcp.setRawX(self.movedGcp.rawX())
                self.manualGcp.setRawY(self.movedGcp.rawY())
                gcp = self.gcpItems[self.refMoved]
                self.deleteGcp(self.refMoved)
                self.autoGcpManager.addGcp(self.manualGcp)
                self.gcpItems.append(gcpItem)
                self.drawGcp(gcpItem, point, 0)
                self.drawGcp(gcpItem, gcp.getRawCenter(), 1)
                self.updateGcpTable()
                self.refMoved = -1
        except:
          QgsLogger.debug( "Handling reference GCP drawing failed: ", 1, "guibase.py", "Ui_MainWindow::handleRefGcpDrawing()")

  def handleRawGcpDrawing(self, point, button):
        try:
          gcpItem = self.gcpItems[len(self.gcpItems)-1]
          if self.ui.actionAddGCP.isChecked():
            gcpItem.setMatch(-1)
            self.drawGcp(gcpItem, point, 1)
            self.updateGcpTable()
            self.ui.actionZoomOut.setChecked(False)
            self.ui.actionZoomIn.setChecked(False)
            self.ui.actionPanImage.setChecked(False)
            self.ui.actionRemoveGCP.setChecked(False)
            self.ui.actionMoveGCP.setChecked(False)
            self.ui.rawMapCanvas.unsetMapTool(self.rawEmitPoint)
            self.ui.refMapCanvas.setMapTool(self.refEmitPoint)
            self.manualGcp.setRawX(point.x())
            self.manualGcp.setRawY(point.y())
            self.autoGcpManager.updateRawGcp(self.manualGcp)
          elif self.ui.actionRemoveGCP.isChecked():
            for i in range(0, len(self.gcpItems)):
              rawCenter = self.gcpItems[i].getRawCanvasPoint()
              rawPoint = QgsVertexMarker(self.ui.rawMapCanvas).toCanvasCoordinates(point)
              if (rawPoint.x() >= rawCenter.x()) and (rawPoint.x() < (rawCenter.x()+22)) and (rawPoint.y() <= rawCenter.y()) and (rawPoint.y() > (rawCenter.y()-28)):
                self.deleteGcp(i)
                self.updateGcpTable()
                break
          elif self.ui.actionMoveGCP.isChecked():
            if self.rawMoved == -1:
              for i in range(0, len(self.gcpItems)):
                rawCenter = self.gcpItems[i].getRawCanvasPoint()
                rawPoint = QgsVertexMarker(self.ui.rawMapCanvas).toCanvasCoordinates(point)
                if (rawPoint.x() >= rawCenter.x()) and (rawPoint.x() < (rawCenter.x()+22)) and (rawPoint.y() <= rawCenter.y()) and (rawPoint.y() > (rawCenter.y()-28)):
                  self.ui.rawMapCanvas.scene().removeItem(self.gcpItems[i].getRawMarker())
                  self.updateGcpTable()
                  self.rawMoved = i
                  self.movedGcp = self.autoGcpManager.gcpSet().constList()[i]
                  break
            else:
                gcpItem = AutoGcpItem(self.ui.refMapCanvas, self.ui.rawMapCanvas)
                gcpItem.setMatch(-1)
                self.manualGcp = QgsGcp()
                self.manualGcp.setRawX(point.x())
                self.manualGcp.setRawY(point.y())
                self.manualGcp.setRefX(self.movedGcp.refX())
                self.manualGcp.setRefY(self.movedGcp.refY())
                gcp = self.gcpItems[self.rawMoved]
                self.deleteGcp(self.rawMoved)
                self.autoGcpManager.addGcp(self.manualGcp)
                self.gcpItems.append(gcpItem)
                self.drawGcp(gcpItem, point, 1)
                self.drawGcp(gcpItem, gcp.getRefCenter(), 0)
                self.updateGcpTable()
                self.rawMoved = -1
        except:
          QgsLogger.debug( "Handling raw GCP drawing failed", 1, "guibase.py", "Ui_MainWindow::handleRawGcpDrawing()")

  def importGcps(self):
        try:
          if len(self.refFilePath) > 0:
            self.deleteAllGcps()
            dialog = QFileDialog(None, "Output File", self.settings.lastOpenRefPath)
            dialog.setFileMode(QFileDialog.ExistingFile)
            if dialog.exec_() == QDialog.Accepted:
              files = dialog.selectedFiles()
              outputPath = files[0]
              success = self.autoGcpManager.importGcpSet(outputPath)
              gcpList = self.autoGcpManager.gcpSet().constList()
              for i in range(0, len(gcpList)):
                gcpPoint = QgsPoint(gcpList[i].refX(), gcpList[i].refY())
                gcpItem = AutoGcpItem(self.ui.refMapCanvas, self.ui.rawMapCanvas)
                gcpItem.setMatch(-1)
                self.gcpItems.append(gcpItem)
                self.drawGcp(gcpItem, gcpPoint, 0)
                if len(self.rawFilePath) > 0:
                  gcpPoint2 = QgsPoint(gcpList[i].rawX(), gcpList[i].rawY())
                  self.drawGcp(gcpItem, gcpPoint2, 1)
              self.updateGcpTable()
              if len(gcpList) < 0 or not success:
                self.showMessage("The GCPs could not be imported.\n\nSuggested intput file:\n"+tempPath, "warning")
              else:
                self.showMessage("The GCPs were successfully imported.\n\nIutput file:\n"+tempPath, "info")
              self.showStatus("GCPs imported", 3)
        except:
          QgsLogger.debug( "Importing GCPs failed", 1, "guibase.py", "Ui_MainWindow::importGcps()")

  def exportGcps(self):
        try:
          tempPath = QFileDialog.getSaveFileName(self, "Export GCP set", self.refFilePath+".points", "GCP file(*.points);;Text files (*.txt)")
          if len(tempPath) > 0:
            if self.autoGcpManager.exportGcpSet(tempPath):
              self.showMessage("The GCPs were successfully exported.\n\nOutput file:\n"+tempPath, "info")
            else:
              self.showMessage("The GCPs could not be exported.\n\nSuggested output file:\n"+tempPath, "warning")
        except:
          QgsLogger.debug( "Exporting GCPs failed", 1, "guibase.py", "Ui_MainWindow::exportGcps()")

  def changeSettings(self):
        try:
          self.settingsDialog = SettingsDialog(self)
          self.settingsDialog.setUserSettings(self.settings)
          try:
            self.settingsDialog.setBandCount(int(self.bandCount))
          except:
            self.settingsDialog.setBandCount(0)
          QObject.connect(self.settingsDialog, SIGNAL("accepted()"), self.applySettings)
          self.settingsDialog.show()
        except:
          QgsLogger.debug( "Changing the user settings failed", 1, "guibase.py", "Ui_MainWindow::changeSettings()")

  def applySettings(self):
        try:
          self.autoGcpManager.setChipSize(self.settings.chipWidth, self.settings.chipHeight)
          self.ui.refMapCanvas.setCanvasColor(QColor(self.settings.refCanvasColor))
          self.ui.rawMapCanvas.setCanvasColor(QColor(self.settings.rawCanvasColor))
          for i in range(0, len(self.gcpHighlights)):
            self.ui.refMapCanvas.scene().removeItem(self.gcpHighlights[i].getRefMarker())
            self.ui.rawMapCanvas.scene().removeItem(self.gcpHighlights[i].getRawMarker())
          for i in range(0, len(self.gcpItems)):
            self.ui.refMapCanvas.scene().removeItem(self.gcpItems[i].getRefMarker())
            if self.gcpItems[i].hasRaw():
              self.ui.rawMapCanvas.scene().removeItem(self.gcpItems[i].getRawMarker())
          for i in range(0, len(self.gcpItems)):
            gcpItem = AutoGcpItem(self.ui.refMapCanvas, self.ui.rawMapCanvas)
            gcpItem.setMatch(self.gcpItems[i].getMatch())
            self.drawGcp(gcpItem, self.gcpItems[i].getRefCenter(), 0)
            if self.gcpItems[i].hasRaw():
              self.drawGcp(gcpItem, self.gcpItems[i].getRawCenter(), 1)
            self.gcpItems[i] = gcpItem
          self.updateGcpTable()
          self.ui.refMapCanvas.refresh()
          self.ui.rawMapCanvas.refresh()
        except:
          QgsLogger.debug( "Applying the user settings failed", 1, "guibase.py", "Ui_MainWindow::applySettings()")

  def setRawExtents(self):
        try:
          if self.ui.actionZoomInAll.isChecked() or self.ui.actionZoomOutAll.isChecked() or self.ui.actionPanAll.isChecked():
            if self.extentsSet == 0:
              self.extentsSet = 1
              self.ui.rawMapCanvas.setExtent(self.ui.refMapCanvas.extent())
              self.ui.rawMapCanvas.refresh()
            else:
              self.extentsSet = 0
        except:
          QgsLogger.debug( "Setting the raw image extent failed", 1, "guibase.py", "Ui_MainWindow::setRawExtents()")

  def setRefExtents(self):
        try:
          if self.ui.actionZoomInAll.isChecked() or self.ui.actionZoomOutAll.isChecked() or self.ui.actionPanAll.isChecked():
            if self.extentsSet == 0:
              self.extentsSet = 1
              self.ui.refMapCanvas.setExtent(self.ui.rawMapCanvas.extent())
              self.ui.refMapCanvas.refresh()
            else:
              self.extentsSet = 0
        except:
          QgsLogger.debug( "Setting the reference image extent failed: ", 1, "guibase.py", "Ui_MainWindow::setRefExtents()")

  def zoomImage(self):
        try:
          self.ui.refMapCanvas.zoomToFullExtent()
          self.ui.rawMapCanvas.zoomToFullExtent()
        except:
          QgsLogger.debug( "Zooming to image failed", 1, "guibase.py", "Ui_MainWindow::zoomImage()")

  def zoomInAll(self):
        try:
          self.ui.refMapCanvas.setMapTool(self.refToolZoomIn)
          self.ui.rawMapCanvas.setMapTool(self.rawToolZoomIn)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomIn.setChecked(False)
          self.ui.actionZoomOut.setChecked(False)
          self.ui.actionPanImage.setChecked(False)
          self.ui.actionZoomIn2.setChecked(False)
          self.ui.actionZoomOut2.setChecked(False)
          self.ui.actionPanImage2.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
        except:
          QgsLogger.debug( "Zooming in onto both images failed", 1, "guibase.py", "Ui_MainWindow::zoomInAll()")

  def zoomOutAll(self):
        try:
          self.ui.refMapCanvas.setMapTool(self.refToolZoomOut)
          self.ui.rawMapCanvas.setMapTool(self.rawToolZoomOut)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomIn.setChecked(False)
          self.ui.actionZoomOut.setChecked(False)
          self.ui.actionPanImage.setChecked(False)
          self.ui.actionZoomIn2.setChecked(False)
          self.ui.actionZoomOut2.setChecked(False)
          self.ui.actionPanImage2.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
        except:
          QgsLogger.debug( "Zooming out of both images failed", 1, "guibase.py", "Ui_MainWindow::zoomOutAll()")

  def panAll(self):
        try:
          self.ui.refMapCanvas.setMapTool(self.refToolPan)
          self.ui.rawMapCanvas.setMapTool(self.rawToolPan)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomIn.setChecked(False)
          self.ui.actionZoomOut.setChecked(False)
          self.ui.actionPanImage.setChecked(False)
          self.ui.actionZoomIn2.setChecked(False)
          self.ui.actionZoomOut2.setChecked(False)
          self.ui.actionPanImage2.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
        except:
          QgsLogger.debug( "Panning both images failed", 1, "guibase.py", "Ui_MainWindow::panAll()")

  def zoomIn(self):
        try:
          self.ui.refMapCanvas.setMapTool(self.refToolZoomIn)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
          self.ui.actionZoomOut.setChecked(False)
          self.ui.actionPanImage.setChecked(False)
        except:
          QgsLogger.debug( "Zooming in onto the reference image failed", 1, "guibase.py", "Ui_MainWindow::zoomIn()")

  def zoomOut(self):
        try:
          self.ui.refMapCanvas.setMapTool(self.refToolZoomOut)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
          self.ui.actionZoomIn.setChecked(False)
          self.ui.actionPanImage.setChecked(False)
        except:
          QgsLogger.debug( "Zooming out of the reference image failed: ", 1, "guibase.py", "Ui_MainWindow::zoomOut()")

  def pan(self):
        try:
          self.ui.refMapCanvas.setMapTool(self.refToolPan)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
          self.ui.actionZoomIn.setChecked(False)
          self.ui.actionZoomOut.setChecked(False)
        except:
          QgsLogger.debug( "Panning the reference image failed", 1, "guibase.py", "Ui_MainWindow::pan()")

  def zoomIn2(self):
        try:
          self.ui.rawMapCanvas.setMapTool(self.rawToolZoomIn)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
          self.ui.actionZoomOut2.setChecked(False)
          self.ui.actionPan2.setChecked(False)
        except:
          QgsLogger.debug( "Zooming in onto the raw image failed", 1, "guibase.py", "Ui_MainWindow::zoomIn2()")

  def zoomOut2(self):
        try:
          self.ui.rawMapCanvas.setMapTool(self.rawToolZoomOut)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
          self.ui.actionZoomIn2.setChecked(False)
          self.ui.actionPan2.setChecked(False)
        except:
          QgsLogger.debug( "Zooming out of the raw image failed", 1, "guibase.py", "Ui_MainWindow::zoomOut2()")

  def pan2(self):
        try:
          self.ui.rawMapCanvas.setMapTool(self.rawToolPan)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
          self.ui.actionZoomIn2.setChecked(False)
          self.ui.actionZoomOut2.setChecked(False)
        except:
          QgsLogger.debug( "Panning the raw image failed", 1, "guibase.py", "Ui_MainWindow::pan2()")

  def handleAddGCP(self):
        try:
          self.ui.refMapCanvas.setMapTool(self.refEmitPoint)
          self.ui.actionZoomOut.setChecked(False)
          self.ui.actionZoomIn.setChecked(False)
          self.ui.actionPanImage.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
        except:
          QgsLogger.debug( "Adding GCP failed", 1, "guibase.py", "Ui_MainWindow::handleAddGCP()")

  def handleRemoveGCP(self):
        try:
          self.ui.refMapCanvas.setMapTool(self.refEmitPoint)
          self.ui.rawMapCanvas.setMapTool(self.rawEmitPoint)
          self.ui.actionZoomOut.setChecked(False)
          self.ui.actionZoomIn.setChecked(False)
          self.ui.actionPanImage.setChecked(False)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionMoveGCP.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
        except:
          QgsLogger.debug( "Removing GCP failed", 1, "guibase.py", "Ui_MainWindow::handleRemoveGCP()")

  def drawGcp(self, gcpItem, point, canvasChoice):
        try:
          if canvasChoice == 0:
            gcpItem.setId(len(self.gcpItems))
            self.newPoint = AutoGcpPoint()
            self.newPoint.setX(point.x())
            self.newPoint.setY(point.y())
            gcpItem.setRefCenter(self.newPoint)
            gcpItem.getRefMarker().setIcon(self.settings.markerIconType, self.settings.markerIconColor)
            gcpItem.getRefMarker().show()
          else:
            self.newPoint = AutoGcpPoint()
            self.newPoint.setX(point.x())
            self.newPoint.setY(point.y())
            gcpItem.setRawCenter(self.newPoint)
            gcpItem.getRawMarker().setIcon(self.settings.markerIconType, self.settings.markerIconColor)
            gcpItem.getRawMarker().show()
        except:
          QgsLogger.debug( "Drawing GCP failed", 1, "guibase.py", "Ui_MainWindow::drawGcp()")

  def handleMoveGCP(self):
        try:
          self.refMoved = -1
          self.rawMoved = -1
          self.ui.refMapCanvas.setMapTool(self.refEmitPoint)
          self.ui.rawMapCanvas.setMapTool(self.rawEmitPoint)
          self.ui.actionZoomOut.setChecked(False)
          self.ui.actionZoomIn.setChecked(False)
          self.ui.actionPanImage.setChecked(False)
          self.ui.actionRemoveGCP.setChecked(False)
          self.ui.actionAddGCP.setChecked(False)
          self.ui.actionZoomInAll.setChecked(False)
          self.ui.actionZoomOutAll.setChecked(False)
          self.ui.actionPanAll.setChecked(False)
        except:
          QgsLogger.debug( "Moving GCP failed", 1, "guibase.py", "Ui_MainWindow::handleMoveGCP()")

  def clearOldInfo(self):
        try:
          for i in range(0, len(self.gcpItems)):
            self.ui.refMapCanvas.scene().removeItem(self.gcpItems[i].getRefMarker())
            self.ui.rawMapCanvas.scene().removeItem(self.gcpItems[i].getRawMarker())
          self.ui.gcpTable.clearContents()
          self.ui.gcpTable.setRowCount(0)
          self.gcpItems = []
        except:
          QgsLogger.debug( "Clearing the old info failed", 1, "guibase.py", "Ui_MainWindow::clearOldInfo()")

  def updateGcpTable(self):
        try:
          self.ui.gcpTable.clearContents()
          self.ui.gcpTable.setRowCount(len(self.gcpItems));
          self.ui.gcpTable.setColumnCount(7);
          for i in range(0, len(self.gcpItems)):
            gcpItem = self.gcpItems[i]
            gcpItem.setId(i+1)
            self.ui.gcpTable.setItem(i, 0, gcpItem.getRow().getIdItem())
            self.ui.gcpTable.setItem(i, 1, gcpItem.getRow().getSrcXItem())
            self.ui.gcpTable.setItem(i, 2, gcpItem.getRow().getSrcYItem())
            self.ui.gcpTable.setItem(i, 3, gcpItem.getRow().getDstXItem())
            self.ui.gcpTable.setItem(i, 4, gcpItem.getRow().getDstYItem())
            if gcpItem.getMatch() == -1:
              self.ui.gcpTable.setItem(i, 5, QTableWidgetItem("Undefined", QTableWidgetItem.Type))
            else:
              self.ui.gcpTable.setCellWidget(i, 5, gcpItem.getRow().getMatchItem())
            self.ui.gcpTable.setItem(i, 6, gcpItem.getRow().getShowItem())
        except:
          QgsLogger.debug( "Updating the GCP table failed", 1, "guibase.py", "Ui_MainWindow::updateGcpTable()")

  def setGcpVisibility(self, item):
        try:
          if item.column() == 6:
            if item.checkState() == 0:
              try:
                self.gcpItems[item.row()].getRefMarker().hide()
              except:
                empty = 0
              try:
                self.gcpItems[item.row()].getRawMarker().hide()
              except:
                empty = 0
              for i in range(0, len(self.gcpHighlights)):
                self.ui.refMapCanvas.scene().removeItem(self.gcpHighlights[i].getRefMarker())
                self.ui.rawMapCanvas.scene().removeItem(self.gcpHighlights[i].getRawMarker())
              self.ui.refMapCanvas.refresh()
              self.ui.rawMapCanvas.refresh()
              self.gcpHighlights = []
            elif item.checkState() == 2:
              try:
                self.gcpItems[item.row()].getRefMarker().show()
              except:
                empty = 0
              try:
                self.gcpItems[item.row()].getRawMarker().show()
              except:
                empty = 0
              self.ui.gcpTable.setCurrentCell(-1,-1)
              self.ui.gcpTable.clearSelection()
        except:
          QgsLogger.debug( "Setting the GCP visibility failed", 1, "guibase.py", "Ui_MainWindow::setGcpVisibility()")

  def saveDatabase(self):
      try:
        if len(self.refFilePath) > 0 or len(self.rawFilePath) > 0:
          if self.autoGcpManager.saveDatabase():
            self.showMessage("The GCPs were saved to the database.", "info")
          else:
            self.showMessage("The GCPs could not be saved to the database.", "warning")
        else:
          try:
            if self.autoGcpManager.gcpSet().size() < 1:
              self.showMessage("There are no GCPs to be saved to the database.", "warning")
          except:
            self.showMessage("There are no GCPs to be saved to the database.", "warning")
      except:
        QgsLogger.debug( "Saving to the database failed", 1, "guibase.py", "Ui_MainWindow::saveDatabase()")

  def loadDatabase(self):
      try:
        if len(self.refFilePath) > 0 or len(self.rawFilePath) > 0:
          self.dbDialog = DatabaseDialog(self)
          if len(self.refFilePath) > 0 and len(self.rawFilePath) > 0:
            self.dbDialog.setFilter(2)
          elif len(self.refFilePath) > 0:
            self.dbDialog.setFilter(0)
          elif len(self.rawFilePath) > 0:
            self.dbDialog.setFilter(1)
          QObject.connect(self.dbDialog, SIGNAL("accepted()"), self.applyDatabaseQuery)
          self.loadingOption = 0
          self.imageHash = ""
          self.dbDialog.exec_()
        else:
          self.showMessage("Please open a reference or raw image.", "info")
      except:
        QgsLogger.debug( "Loading from the database failed: ", 1, "guibase.py", "Ui_MainWindow::loadDatabase()")

  def applyDatabaseQuery(self):
      try:
        self.imageFilter = self.dbUi.getFilter()
        self.loadingOption = self.dbUi.getLoadingOption()
        self.imageHash = self.dbUi.getImageHash()
        self.deleteAllGcps()
        if self.loadingOption == 0:
          if len(self.rawFilePath) > 0:
            self.loadGcps(1, self.loadingOption)
          elif len(self.refFilePath) > 0:
            self.loadGcps(0, self.loadingOption)
          else:
            self.showMessage("Please open a reference image.", "info")
        elif self.loadingOption == 1:
          self.loadGcps(0, self.loadingOption)
        elif self.loadingOption == 2:
          self.loadGcps(0, self.loadingOption)
      except:
        QgsLogger.debug( "Applying query to the database failed", 1, "guibase.py", "Ui_MainWindow::applyDatabaseQuery()")

    #FoxHat Added - End
