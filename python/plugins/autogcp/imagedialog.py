#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
imagedialogbase.py - The class providing the graphical user interface for
the AutoGCP plugin's Image dialog. All the available information regarding
the image is presented by this dialog.
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
from qgis.analysis import *
from qgis.core import *
from qgis.gui import *
from helpviewer import *
from ui_imagedialogbase import Ui_ImageDialog

class ImageDialog(QDialog):
  
  def __init__(self, parent):
    QDialog.__init__( self, parent )
    self.ui = Ui_ImageDialog()
    self.ui.setupUi( self )
    try:
      self.ui.tabWidget.setCurrentIndex(0)

      pal = self.ui.fileName.palette()
      pal.setColor(QPalette.Base, Qt.transparent)

      self.ui.fileName.setPalette(pal)
      self.ui.fileName.setPalette(pal)
      self.ui.hash.setPalette(pal)
      self.ui.filePath.setPalette(pal)
      self.ui.filePath.setPalette(pal)
      self.ui.fileFormat.setPalette(pal)
      self.ui.fileFormat.setPalette(pal)
      self.ui.fileSize.setPalette(pal)
      self.ui.fileSize.setPalette(pal)
      self.ui.rasterWidth.setPalette(pal)
      self.ui.rasterWidth.setPalette(pal)
      self.ui.rasterHeight.setPalette(pal)
      self.ui.rasterHeight.setPalette(pal)
      self.ui.originCoordinates.setPalette(pal)
      self.ui.originCoordinates.setPalette(pal)
      self.ui.pixelSize.setPalette(pal)
      self.ui.pixelSize.setPalette(pal)
      self.ui.rasterBands.setPalette(pal)
      self.ui.rasterBands.setPalette(pal)
      self.ui.dateCreated.setPalette(pal)
      self.ui.dateModified.setPalette(pal)
      self.ui.dateRead.setPalette(pal)

      screen = QtGui.QApplication.desktop();
      width = self.width();
      height = self.height();
      x = (screen.screenGeometry(screen.primaryScreen()).width() - width) / 2
      y = (screen.screenGeometry(screen.primaryScreen()).height() - height) / 2;
      self.move(x, y);
      QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Help), SIGNAL("clicked()"), self.showHelp)
      QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Apply), SIGNAL("clicked()"), self.applySettings)
      QObject.connect(self.ui.buttonBox.button(QDialogButtonBox.Close), SIGNAL("clicked()"), self.close)
      QObject.connect(self.ui.radioGray, SIGNAL("clicked()"), self.changeColorSelection)
      QObject.connect(self.ui.radioRgb, SIGNAL("clicked()"), self.changeColorSelection)
      QObject.connect(self.ui.grayColorMap, SIGNAL("currentIndexChanged(int)"), self.changeColorSelection)
    except:
      QgsLogger.debug( "Setting up the dialog failed: ", 1, "imagedialog.py", "ImageDialog::__init__()")

  def showHelp(self):
    try:
      self.helpDialog = HelpDialog(self)
      self.helpDialog.loadContext("ImageInfoDialog")
      self.helpDialog.exec_()
    except:
      QgsLogger.debug( "Showing help failed", 1, "imagedialog.py", "ImageDialog::showHelp()")

  def setProjection(self, manager, image, srsId, epsg):
    try:
      self.manager = manager
      self.image = image
      self.ui.widget.setSelectedCrsId(srsId)
    except:
      QgsLogger.debug( "Setting projection failed", 1, "imagedialog.py", "ImageDialog::setProjection()")

  def setComponents(self, layer, canvas):
    try:
      self.layer = layer
      self.canvas = canvas
      self.ui.radioRgb.setChecked(True)
      self.changeColorSelection()
      for i in range(0, self.layer.bandCount()):
	self.ui.redBand.addItem("Band "+str(i+1))
        self.ui.greenBand.addItem("Band "+str(i+1))
        self.ui.blueBand.addItem("Band "+str(i+1))
        self.ui.grayBand.addItem("Band "+str(i+1))
      self.ui.redBand.addItem("Not Set")
      self.ui.greenBand.addItem("Not Set")
      self.ui.blueBand.addItem("Not Set")
      self.ui.grayBand.addItem("Not Set")

      self.ui.checkInvert.setChecked(self.layer.invertHistogram())

      if self.layer.contrastEnhancementAlgorithm() == QgsContrastEnhancement.StretchToMinimumMaximum:
	self.ui.contrast.setCurrentIndex(1)
      elif self.layer.contrastEnhancementAlgorithm() == QgsContrastEnhancement.StretchAndClipToMinimumMaximum:
	self.ui.contrast.setCurrentIndex(2)
      elif self.layer.contrastEnhancementAlgorithm() == QgsContrastEnhancement.ClipToMinimumMaximum:
	self.ui.contrast.setCurrentIndex(3)
      elif QgsContrastEnhancement.UserDefinedEnhancement == self.layer.contrastEnhancementAlgorithm():
	pass
      else:
	self.ui.contrast.setCurrentIndex(0)

      if self.layer.drawingStyle() == QgsRasterLayer.SingleBandPseudoColor:
	self.ui.grayColorMap.setCurrentIndex(0)

      if self.layer.colorShadingAlgorithm() == QgsRasterLayer.PseudoColorShader:
	self.ui.grayColorMap.setCurrentIndex(1)
      elif self.layer.colorShadingAlgorithm() == QgsRasterLayer.FreakOutShader:
	self.ui.grayColorMap.setCurrentIndex(2)

      for i in range(0, self.ui.redBand.count()):
	if self.ui.redBand.itemText(i) == self.layer.redBandName():
	  self.ui.redBand.setCurrentIndex(i)
      for i in range(0, self.ui.greenBand.count()):
	if self.ui.greenBand.itemText(i) == self.layer.greenBandName():
	  self.ui.greenBand.setCurrentIndex(i)
      for i in range(0, self.ui.blueBand.count()):
	if self.ui.blueBand.itemText(i) == self.layer.blueBandName():
	  self.ui.blueBand.setCurrentIndex(i)
      for i in range(0, self.ui.grayBand.count()):
	if self.ui.grayBand.itemText(i) == self.layer.grayBandName():
	  self.ui.grayBand.setCurrentIndex(i)
    except:
      QgsLogger.debug( "Setting components failed: ", 1, "imagedialog.py", "ImageDialog::setComponents()")

  def changeColorSelection(self):
    try:
      if self.ui.radioRgb.isChecked():
	self.ui.groupRgb.show()
        self.ui.groupContrast.show()
        self.ui.groupGray.hide()
      elif self.ui.radioGray.isChecked():
	self.ui.groupRgb.hide()
        self.ui.groupGray.show()
	if self.ui.grayColorMap.currentText() == "Grayscale":
          self.ui.groupContrast.show()
        else:
	  self.ui.groupContrast.hide()
    except:
      QgsLogger.debug( "Changing color selection failed", 1, "imagedialog.py", "ImageDialog::changeColorSelection()")

  def applySettings(self):
    try:
      try:
	if self.layer.dataProvider() == 0:
	  self.mRasterLayerIsGdal = True
        elif self.layer.dataProvider().name() == "wms":
	  self.mRasterLayerIsGdal = False
      except:
	self.mRasterLayerIsGdal = False
        QgsLogger.debug("Not GDAL supported layer")

      #Apply the projection
      self.manager.setProjection(self.image, self.ui.widget.selectedProj4String())

      #If gray band is selected
      QgsLogger.debug( "Apply processing symbology tab" )
      if self.ui.radioGray.isChecked():
	if self.layer.rasterType() == QgsRasterLayer.GrayOrUndefined:
          if self.ui.grayColorMap.currentText() != "Grayscale":
            QgsLogger.debug( "Raster Drawing Style to :: SingleBandPseudoColor" )
            self.layer.setDrawingStyle( QgsRasterLayer.SingleBandPseudoColor )
          else:
            QgsLogger.debug( "Setting Raster Drawing Style to :: SingleBandGray" )
            self.layer.setDrawingStyle( QgsRasterLayer.SingleBandGray )
        elif self.layer.rasterType() == QgsRasterLayer.Palette:
          if self.ui.grayColorMap.currentText() == "Grayscale":
            QgsLogger.debug( "Setting Raster Drawing Style to :: PalettedSingleBandGray" )
            QgsLogger.debug( QString( "Combo value : %1 GrayBand Mapping : %2" ).arg( self.ui.grayBand.currentText() ).arg( self.layer.grayBandName() ) )
            self.layer.setDrawingStyle( QgsRasterLayer.PalettedSingleBandGray )
          else:
            QgsLogger.debug( "Setting Raster Drawing Style to :: PalettedSingleBandPseudoColor" )
            self.layer.setDrawingStyle( QgsRasterLayer.PalettedSingleBandPseudoColor )
	elif self.layer.rasterType() == QgsRasterLayer.Multiband:
	  if self.ui.grayColorMap.currentText() != "Grayscale":
	    QgsLogger.debug( "Setting Raster Drawing Style to ::MultiBandSingleBandPseudoColor " )
	    self.layer.setDrawingStyle( QgsRasterLayer.MultiBandSingleBandPseudoColor )
	  else:
	    QgsLogger.debug( "Setting Raster Drawing Style to :: MultiBandSingleBandGray" )
	    QgsLogger.debug( QString( "Combo value : %1 GrayBand Mapping : %2" ).arg( self.ui.grayBand.currentText() ).arg( self.layer.grayBandName() ) )
	    self.layer.setDrawingStyle( QgsRasterLayer.MultiBandSingleBandGray )
	else:
	  if self.layer.rasterType() == QgsRasterLayer.Palette:
	    QgsLogger.debug( "Setting Raster Drawing Style to :: PalettedMultiBandColor" )
	    self.layer.setDrawingStyle( QgsRasterLayer.PalettedMultiBandColor )
	  elif self.layer.rasterType() == QgsRasterLayer.Multiband:
	    QgsLogger.debug( "Setting Raster Drawing Style to :: MultiBandColor" )
	    self.layer.setDrawingStyle( QgsRasterLayer.MultiBandColor )
      else:
	if self.layer.rasterType() == QgsRasterLayer.Palette:
	  QgsLogger.debug( "Setting Raster Drawing Style to :: PalettedMultiBandColor" )
	  self.layer.setDrawingStyle( QgsRasterLayer.PalettedMultiBandColor )
	elif self.layer.rasterType() == QgsRasterLayer.Multiband:
	  QgsLogger.debug( "Setting Raster Drawing Style to :: MultiBandColor" )
	  self.layer.setDrawingStyle( QgsRasterLayer.MultiBandColor )
      
      #Invert the layer colors
      if self.ui.checkInvert.isChecked():
        self.layer.setInvertHistogram(True)
      else:
        self.layer.setInvertHistogram(False)

      self.layer.setRedBandName( self.ui.redBand.currentText() )
      self.layer.setGreenBandName( self.ui.greenBand.currentText() )
      self.layer.setBlueBandName( self.ui.blueBand.currentText() )
      self.layer.setGrayBandName( self.ui.grayBand.currentText() )

      if self.ui.grayColorMap.currentText() == "Pseudocolor":
        self.layer.setColorShadingAlgorithm( QgsRasterLayer.PseudoColorShader )
      elif self.ui.grayColorMap.currentText() == "Freak Out":
        self.layer.setColorShadingAlgorithm( QgsRasterLayer.FreakOutShader )

      if self.ui.contrast.currentText() == "Stretch To MinMax":
        self.layer.setContrastEnhancementAlgorithm( QgsContrastEnhancement.StretchToMinimumMaximum, False )
      elif self.ui.contrast.currentText() == "Stretch And Clip To MinMax":
        self.layer.setContrastEnhancementAlgorithm( QgsContrastEnhancement.StretchAndClipToMinimumMaximum, False )
      elif self.ui.contrast.currentText() == "Clip To MinMax":
        self.layer.setContrastEnhancementAlgorithm( QgsContrastEnhancement.ClipToMinimumMaximum, False )
      elif QgsContrastEnhancement.UserDefinedEnhancement == self.layer.contrastEnhancementAlgorithm():
	pass
      else:
        self.layer.setContrastEnhancementAlgorithm( QgsContrastEnhancement.NoEnhancement, False )

      if self.mRasterLayerIsGdal and self.ui.radioRgb.isChecked():
        self.layer.setStandardDeviations( 0.0 )
        self.layer.setUserDefinedRGBMinimumMaximum( False )

      self.canvas.refresh()
    except:
      QgsLogger.debug( "Applying settings failed", 1, "imagedialog.py", "ImageDialog::applySettings()")

  def setImageInfo(self, info):
    try:
      self.ui.fileName.setText(info.pFileName)
      self.ui.fileName.setCursorPosition(0)
      self.ui.filePath.setText(info.pFilePath)
      self.ui.filePath.setCursorPosition(0)
      self.ui.hash.setText(info.pHash)
      self.ui.fileFormat.setText(info.pFileFormat)
      self.ui.fileFormat.setCursorPosition(0)
      self.ui.fileSize.setText(info.pFileSize)
      self.ui.fileSize.setCursorPosition(0)
      self.ui.rasterWidth.setText(info.pRasterWidth)
      self.ui.rasterWidth.setCursorPosition(0)
      self.ui.rasterHeight.setText(info.pRasterHeight)
      self.ui.rasterHeight.setCursorPosition(0)
      self.ui.originCoordinates.setText(info.pOriginCoordinates)
      self.ui.originCoordinates.setCursorPosition(0)
      self.ui.pixelSize.setText(info.pPixelSize)
      self.ui.pixelSize.setCursorPosition(0)
      self.ui.rasterBands.setText(info.pRasterBands)
      self.ui.rasterBands.setCursorPosition(0)
      self.ui.dateCreated.setDateTime(info.pDateCreated)
      self.ui.dateModified.setDateTime(info.pDateModified)
      self.ui.dateRead.setDateTime(info.pDateRead)
    except:
      QgsLogger.debug( "Setting the image information failed", 1, "imagedialog.py", "ImageDialog::setImageInf()")