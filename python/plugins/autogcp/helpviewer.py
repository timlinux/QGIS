#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
helpviewer.py - The class providing the graphical user interface for the
AutoGCP help viewer. All Help content will be displayed using this interface.
--------------------------------------
Date : 10-October-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
Modified: Christoph Stallmann
Modification Date: 10/10/2010
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
from qgis.gui import *
from ui_helpviewerbase import Ui_HelpViewer

class HelpDialog(QDialog):
  
  def __init__(self, parent):
    QDialog.__init__( self, parent )
    self.ui = Ui_HelpViewer()
    self.ui.setupUi( self )

  def loadContext(self, contextId):
    try:
      if len(contextId) > 0:
	#set up the path to the help file
        helpFilesPath = QgsApplication.pkgDataPath() + "/python/plugins/autogcp/help/" + contextId
        helpContents = ""
        file = QFile(helpFilesPath);
        #check to see if the localized version exists
        if not file.exists():
	  helpContents = "No help file exists for this context.\n"+helpFilesPath
        elif not file.open( QIODevice.ReadOnly or QIODevice.Text ):
	  helpContents = "The help file could not be opened.\n"+helpFilesPath
        else:
	  inStream = QTextStream(file)
          inStream.setCodec( "UTF-8" ) #Help files must be in Utf-8
          while not inStream.atEnd():
	    line = inStream.readLine()
            helpContents = helpContents + line
          file.close()
        #Set the browser text to the help contents
        myStyle = QgsApplication.reportStyleSheet()
        helpContents = "<head><style>" + myStyle + "</style></head><body>" + helpContents + "</body>"
        self.ui.webView.setHtml( helpContents )
    except:
      QgsLogger.debug( "Loading help context failed", 1, "helpviewer.py", "HelpViewer::loadContext()")