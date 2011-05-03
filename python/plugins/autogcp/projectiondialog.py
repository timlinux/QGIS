#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
projectiondialogbase.py - The class providing the graphical user interface
for the AutoGCP plugin's Projection dialog. This is an input dialog for the
necessary geographic and projected coordinate system. If the projection
information could not be extracted from the image metadata, this dialog is
presented to the user.
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
from qgis.analysis import *
from qgis.core import *
from ui_projectiondialogbase import Ui_ProjectionDialog

class ProjectionDialog(QDialog):
  
  def __init__(self, parent):
    QDialog.__init__( self, parent )
    self.ui = Ui_ProjectionDialog()
    self.ui.setupUi( self )
    try:
      QObject.connect(self.ui.buttonBox, SIGNAL("rejected()"), self.close)
      QObject.connect(self.ui.buttonBox, SIGNAL("accepted()"), self.accept)

      screen = QtGui.QApplication.desktop()
      self.resize(screen.width()/2, screen.height()-(screen.height()*0.15))
      width = self.width()
      height = self.height()
      x = (screen.screenGeometry(screen.primaryScreen()).width() - width) / 2
      y = (screen.screenGeometry(screen.primaryScreen()).height() - height) / 2
      self.move(x, y)

    except:
      QgsLogger.debug( "Setting up the dialog failed", 1, "projectiondialog.py", "ProjectionDialog::__init__()")

  def getProjectionString(self):
    return self.ui.widget.selectedProj4String()

  def getProjectionName(self):
    return self.ui.widget.selectedName()

