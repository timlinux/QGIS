#!python
# -*- coding: utf-8 -*-
"""
***************************************************************************************
sumbandilasat.py - The main interface for the SumbandilaSat Level 3 System plug-in.
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

from PyQt4.QtCore import *
from PyQt4.QtGui import *
import qgis
from qgis.core import *
from qgis.gui import *
from processor import ProcessorDialog
from tools.georeferencer.georeferencer import GeoreferencerWindow
from tools.columncorrector.columncorrector import ColumnCorrectorWindow
from tools.linecorrector.linecorrector import LineCorrectorWindow
from tools.cloudcoverer.cloudcoverer import CloudCovererWindow
from tools.bandaligner.bandaligner import BandAlignerWindow
import os.path


class sumbandilasatPlugin:
  def __init__( self, iface ):
    self.iface = iface
    try:
      self.QgisVersion = unicode( QGis.QGIS_VERSION_INT )
    except:
      self.QgisVersion = unicode( QGis.qgisVersion )[ 0 ]
      
    self.columnCorrectorWindow = None

  #Displays a message to the user
  def showMessage(self, string, type = "info"):
    msgbox = QMessageBox(self.iface.mainWindow())
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

  #Get the icon with a specific number and category
  def getIconPath( self, icon, category = "" ):
    pluginPath = QString( os.path.dirname( __file__ ) )
    if QFile.exists( pluginPath + QDir.separator() + QString( "icons" ) + QDir.separator() + QString( category ) + QDir.separator() + QString( icon ) ):
      return pluginPath + QDir.separator() + QString( "icons" ) + QDir.separator() + QString( category ) + QDir.separator() + QString( icon )
    elif QFile.exists( pluginPath + QDir.separator() + QString( "icons" ) + QDir.separator() + QString( icon ) ):
      return pluginPath + QDir.separator() + QString( "icons" ) + QDir.separator() + QString( icon )
    else:
      return ""

  def initGui( self ):
    
    #Create the plug-in tool bar
    self.toolBar = self.iface.addToolBar( "SumbandilaSatToolBar" )
    sizePolicy = QSizePolicy( QSizePolicy.Preferred, QSizePolicy.Fixed )
    sizePolicy.setHorizontalStretch( 0 )
    sizePolicy.setVerticalStretch( 0 )
    sizePolicy.setHeightForWidth( self.toolBar.sizePolicy().hasHeightForWidth() )
    self.toolBar.setSizePolicy( sizePolicy) 
    self.toolBar.setObjectName( "sumbandilasatToolBar" )
    #self.toolBar.setStyleSheet("QToolBar{ border: 2px solid grey; border-radius: 8px; background-color: qradialgradient(spread:pad, cx:0.5, cy:0.5, radius: 1, fx:0.5, fy:0.5, stop: 1 grey, stop: 0 rgba(0, 0, 0, 0)); background-image: url("+self.getIconPath( "sansa.png", "toolbar" )+"); background-repeat:no-repeat; background-position:right centre;}")
    
    #Create the action/buttons for the tool bar
    
    #Processor action
    self.actionProcessor = QAction(self.toolBar)
    self.actionProcessor.setIcon( QIcon( self.getIconPath( "batch.png", "toolbar" ) ) )
    self.actionProcessor.setObjectName( "actionProcessor" )
    self.actionProcessor.setText( QApplication.translate( "SumbandilaSat", "Image Processor", None, QApplication.UnicodeUTF8 ) )
    self.toolBar.addAction( self.actionProcessor )
    QObject.connect( self.actionProcessor, SIGNAL("triggered()"), self.openProcessor )
    
    #Georefrencer action
    self.actionGeoreference = QAction(self.toolBar)
    self.actionGeoreference.setIcon( QIcon( self.getIconPath( "georeferencer.png", "toolbar" ) ) )
    self.actionGeoreference.setObjectName( "actionGeoreferencer" )
    self.actionGeoreference.setText( QApplication.translate( "SumbandilaSat", "Georeference thumbnails", None, QApplication.UnicodeUTF8 ) )
    self.toolBar.addAction( self.actionGeoreference )
    QObject.connect( self.actionGeoreference, SIGNAL("triggered()"), self.openGeoreferencer )

    #ColumnCorrector action
    self.actionColumnCorrector = QAction(self.toolBar)
    self.actionColumnCorrector.setIcon( QIcon( self.getIconPath( "columncorrector.png", "toolbar" ) ) )
    self.actionColumnCorrector.setObjectName( "actionColumnCorrector" )
    self.actionColumnCorrector.setText( QApplication.translate( "SumbandilaSat", "Column Corrector", None, QApplication.UnicodeUTF8 ) )
    self.toolBar.addAction( self.actionColumnCorrector )
    QObject.connect( self.actionColumnCorrector, SIGNAL("triggered()"), self.openColumnCorrector )
    
    #RowCorrector action
    self.actionLineCorrector = QAction(self.toolBar)
    self.actionLineCorrector.setIcon( QIcon( self.getIconPath( "linecorrector.png", "toolbar" ) ) )
    self.actionLineCorrector.setObjectName( "actionLineCorrector" )
    self.actionLineCorrector.setText( QApplication.translate( "SumbandilaSat", "Line Corrector", None, QApplication.UnicodeUTF8 ) )
    self.toolBar.addAction( self.actionLineCorrector )
    QObject.connect( self.actionLineCorrector, SIGNAL("triggered()"), self.openLineCorrector )
    
    #CloudCoverer action
    self.actionCloudCoverer = QAction(self.toolBar)
    self.actionCloudCoverer.setIcon( QIcon( self.getIconPath( "cloudcoverer.png", "toolbar" ) ) )
    self.actionCloudCoverer.setObjectName( "actionCloudCoverer" )
    self.actionCloudCoverer.setText( QApplication.translate( "SumbandilaSat", "Cloud Coverer", None, QApplication.UnicodeUTF8 ) )
    self.toolBar.addAction( self.actionCloudCoverer )
    QObject.connect( self.actionCloudCoverer, SIGNAL("triggered()"), self.openCloudCoverer )
    
    #BandAligner action
    self.actionBandAligner = QAction(self.toolBar)
    self.actionBandAligner.setIcon( QIcon( self.getIconPath( "bandaligner.png", "toolbar" ) ) )
    self.actionBandAligner.setObjectName( "actionBandAligner" )
    self.actionBandAligner.setText( QApplication.translate( "SumbandilaSat", "Band Aligner", None, QApplication.UnicodeUTF8 ) )
    self.toolBar.addAction( self.actionBandAligner )
    QObject.connect( self.actionBandAligner, SIGNAL("triggered()"), self.openBandAligner )

  def openProcessor(self):
    self.processorWindow = ProcessorDialog( self.iface )
    self.processorWindow.show()
  
  def openGeoreferencer(self):
    self.georeferencerWindow = GeoreferencerWindow( self.iface )
    self.georeferencerWindow.show()
    
  def openColumnCorrector(self):
    self.columnCorrectorWindow = ColumnCorrectorWindow( self.iface )
    self.columnCorrectorWindow.show()
    
  def openLineCorrector(self):
    self.lineCorrectorWindow = LineCorrectorWindow( self.iface )
    self.lineCorrectorWindow.show()
  
  def openCloudCoverer(self):
    self.cloudCovererWindow = CloudCovererWindow( self.iface )
    self.cloudCovererWindow.show()
    
  def openBandAligner(self):
    self.bandAlignerWindow = BandAlignerWindow( self.iface )
    self.bandAlignerWindow.show()

  def unload( self ):
    pass
