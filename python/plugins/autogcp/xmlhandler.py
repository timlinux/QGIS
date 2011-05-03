#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
xmlhandler.py - This class provides the functionality to read and write to a
XML file. The class is used to store and retrieve the settings previously
stored by the user
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

from PyQt4 import QtCore, QtGui, QtXml
#FoxHat Added - Start
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtXml import *
from qgis.core import *
from settings import *
#FoxHat Added - End

class AutoGcpXmlHandler():
  
  def __init__(self, path):
    self.path = path
    
  def setSettings(self, obj):
    self.settings = obj
    
  def saveXml(self):
    try:
      xmlFile = QFile(self.path)
      xmlFile.open(QIODevice.WriteOnly)

      xmlWriter = QXmlStreamWriter(xmlFile)
      xmlWriter.setAutoFormatting(True)
      xmlWriter.writeStartDocument()

      xmlWriter.writeStartElement("Settings")

      xmlWriter.writeStartElement("General")

      xmlWriter.writeStartElement("OpenFileDialogs")

      xmlWriter.writeStartElement("ReferenceImage")
      xmlWriter.writeTextElement("Choice", str(self.settings.refPathChoice))
      xmlWriter.writeTextElement("Path", str(self.settings.refPath))
      xmlWriter.writeEndElement() #ReferenceImage

      xmlWriter.writeStartElement("RawImage")
      xmlWriter.writeTextElement("Choice", str(self.settings.rawPathChoice))
      xmlWriter.writeTextElement("Path", str(self.settings.rawPath))
      xmlWriter.writeEndElement() #RawImage

      xmlWriter.writeStartElement("LastOpened")
      xmlWriter.writeTextElement("RefPath", str(self.settings.lastOpenRefPath))
      xmlWriter.writeTextElement("RawPath", str(self.settings.lastOpenRawPath))
      xmlWriter.writeEndElement() #LastOpened

      xmlWriter.writeEndElement() #OpenFileDialogs

      xmlWriter.writeEndElement() #General

      xmlWriter.writeStartElement("Interface")
      xmlWriter.writeStartElement("GcpMarker")

      xmlWriter.writeStartElement("NormalMarker")
      xmlWriter.writeTextElement("IconType", str(self.settings.markerIconType))
      xmlWriter.writeTextElement("IconColor", str(self.settings.markerIconColor))
      xmlWriter.writeEndElement() #NormalMarker

      xmlWriter.writeStartElement("SelectedMarker")
      xmlWriter.writeTextElement("IconType", str(self.settings.selectedIconType))
      xmlWriter.writeTextElement("IconColor", str(self.settings.selectedIconColor))
      xmlWriter.writeEndElement() #SelectedMarker

      xmlWriter.writeEndElement() #GcpMarker
      xmlWriter.writeStartElement("CanvasColor")

      xmlWriter.writeStartElement("RefCanvas")
      xmlWriter.writeTextElement("Red", str(QColor(self.settings.refCanvasColor).red()))
      xmlWriter.writeTextElement("Green", str(QColor(self.settings.refCanvasColor).green()))
      xmlWriter.writeTextElement("Blue", str(QColor(self.settings.refCanvasColor).blue()))
      xmlWriter.writeEndElement() #RefCanvas

      xmlWriter.writeStartElement("RawCanvas")
      xmlWriter.writeTextElement("Red", str(QColor(self.settings.rawCanvasColor).red()))
      xmlWriter.writeTextElement("Green", str(QColor(self.settings.rawCanvasColor).green()))
      xmlWriter.writeTextElement("Blue", str(QColor(self.settings.rawCanvasColor).blue()))
      xmlWriter.writeEndElement() #RawCanvas

      xmlWriter.writeEndElement() #CanvasColor

      xmlWriter.writeStartElement("WidgetVisibility")
      xmlWriter.writeTextElement("GcpTable", str(self.settings.loadGcpTable))
      xmlWriter.writeTextElement("ReferenceCanvas", str(self.settings.loadRefCanvas))
      xmlWriter.writeTextElement("RawCanvas", str(self.settings.loadRawCanvas))

      xmlWriter.writeEndElement() #WidgetVisibility

      xmlWriter.writeEndElement() #Interface

      xmlWriter.writeStartElement("Algorithms")

      xmlWriter.writeStartElement("GcpDetection")
      xmlWriter.writeTextElement("GcpAmount", str(self.settings.gcpAmount))
      xmlWriter.writeTextElement("BandSelection", str(self.settings.bandSelection))
      xmlWriter.writeEndElement() #GcpDetection
      
      xmlWriter.writeStartElement("GcpCorrelation")
      xmlWriter.writeTextElement("GcpCorrelationThreshold", str(self.settings.correlationThreshold))
      xmlWriter.writeEndElement() #GcpCorrelation

      xmlWriter.writeStartElement("GcpChip")
      xmlWriter.writeTextElement("Width", str(self.settings.chipWidth))
      xmlWriter.writeTextElement("Height", str(self.settings.chipHeight))
      xmlWriter.writeEndElement() #GcpChip

      xmlWriter.writeStartElement("RpcModel")
      xmlWriter.writeTextElement("RmsError", str(self.settings.rmsError))
      xmlWriter.writeEndElement() #RpcModel

      xmlWriter.writeEndElement() #Algorithms

      xmlWriter.writeStartElement("Database")

      xmlWriter.writeStartElement("General")
      xmlWriter.writeTextElement("Selection", str(self.settings.databaseSelection))
      xmlWriter.writeTextElement("Loading", str(self.settings.gcpLoading))
      xmlWriter.writeEndElement() #General

      xmlWriter.writeStartElement("Sqlite")
      xmlWriter.writeTextElement("Choice", str(self.settings.sqliteChoice))
      xmlWriter.writeTextElement("Path", str(self.settings.sqlitePath))
      xmlWriter.writeEndElement() #Sqlite

      xmlWriter.writeEndElement() #Database

      xmlWriter.writeEndDocument()

      xmlFile.close()
    except:
      QgsLogger.debug( "Saving the XML settings failed", 1, "xmlhandler.py", "AutoGcpXmlHandler::saveXml()")

  def openXml(self):
    try:
      doc = QDomDocument("settings");
      xmlFile = QFile(self.path)    
      
      if not xmlFile.open(QIODevice.ReadOnly):
	QgsLogger.debug("No permission to open XML settings file: "+self.path)
	return
	
      if not doc.setContent(xmlFile):          
	xmlFile.close()
	return  
    
      xmlFile.close()
      docElement = doc.documentElement()

      general = docElement.elementsByTagName("General").item(0)
      openDialogs = general.namedItem("OpenFileDialogs")
      refDialog = openDialogs.namedItem("ReferenceImage")
      self.settings.refPathChoice = int(refDialog.firstChildElement("Choice").text())
      self.settings.refPath = refDialog.firstChildElement("Path").text()
      rawDialog = openDialogs.namedItem("RawImage")
      self.settings.rawPathChoice = int(rawDialog.firstChildElement("Choice").text())
      self.settings.rawPath = rawDialog.firstChildElement("Path").text()
      lastOpened = openDialogs.namedItem("LastOpened")
      self.settings.lastOpenRefPath = lastOpened.firstChildElement("RefPath").text()
      self.settings.lastOpenRawPath = lastOpened.firstChildElement("RawPath").text()

      interface = docElement.elementsByTagName("Interface").item(0)
      gcpMarker = interface.namedItem("GcpMarker")
      normalMarker = gcpMarker.namedItem("NormalMarker")
      self.settings.markerIconType = int(normalMarker.firstChildElement("IconType").text())
      self.settings.markerIconColor = int(normalMarker.firstChildElement("IconColor").text())
      selectedMarker = gcpMarker.namedItem("SelectedMarker")
      self.settings.selectedIconType = int(selectedMarker.firstChildElement("IconType").text())
      self.settings.selectedIconColor = int(selectedMarker.firstChildElement("IconColor").text())
      canvasColor = interface.namedItem("CanvasColor")
      refCanvas = canvasColor.namedItem("RefCanvas")
      red = int(refCanvas.firstChildElement("Red").text())
      green = int(refCanvas.firstChildElement("Green").text())
      blue = int(refCanvas.firstChildElement("Blue").text())
      self.settings.refCanvasColor = QColor(red, green, blue)
      rawCanvas = canvasColor.namedItem("RawCanvas")
      red = int(rawCanvas.firstChildElement("Red").text())
      green = int(rawCanvas.firstChildElement("Green").text())
      blue = int(rawCanvas.firstChildElement("Blue").text())
      self.settings.rawCanvasColor = QColor(red, green, blue)
      widget = interface.namedItem("WidgetVisibility")
      self.settings.loadGcpTable = int(widget.firstChildElement("GcpTable").text())
      self.settings.loadRefCanvas = int(widget.firstChildElement("ReferenceCanvas").text())
      self.settings.loadRawCanvas = int(widget.firstChildElement("RawCanvas").text())
      
      algorithms = docElement.elementsByTagName("Algorithms").item(0)
      gcpDetection = algorithms.namedItem("GcpDetection")
      self.settings.gcpAmount = int(gcpDetection.firstChildElement("GcpAmount").text())
      gcpCorrelation = algorithms.namedItem("GcpCorrelation")
      self.settings.correlationThreshold = float(gcpCorrelation.firstChildElement("GcpCorrelationThreshold").text())
      self.settings.bandSelection = int(gcpDetection.firstChildElement("BandSelection").text())
      gcpChip = algorithms.namedItem("GcpChip")
      self.settings.chipWidth = int(gcpChip.firstChildElement("Width").text())
      self.settings.chipHeight = int(gcpChip.firstChildElement("Height").text())
      rpcModel = algorithms.namedItem("RpcModel")
      self.settings.rmsError = float(rpcModel.firstChildElement("RmsError").text())
      
      database = docElement.elementsByTagName("Database").item(0)
      generalDb = database.namedItem("General")
      self.settings.databaseSelection = int(generalDb.firstChildElement("Selection").text())
      self.settings.gcpLoading = int(generalDb.firstChildElement("Loading").text())
      sqlite = database.namedItem("Sqlite")
      self.settings.sqliteChoice = int(sqlite.firstChildElement("Choice").text())
      self.settings.sqlitePath = sqlite.firstChildElement("Path").text()
      
    except:
      QgsLogger.debug( "Opening the XML settings failed", 1, "xmlhandler.py", "AutoGcpXmlHandler::openXml()")
    