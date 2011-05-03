#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
progressdialogbase.py - This class provides the interface for a progress
bar. This dialog is used to display calculations that are currently in
progress.
--------------------------------------
Date : 21-October-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
Modified: Christoph Stallmann
Modification Date: 21/10/2010
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
from qgis.analysis import *
from ui_progressdialogbase import Ui_ProgressDialog
import time

class ProgressDialog(QDialog):
  
  def __init__(self, parent):
    QDialog.__init__( self, parent )
    self.ui = Ui_ProgressDialog()
    self.ui.setupUi( self )
    
    self.ui.progressBar.setMinimum(0)
    self.ui.progressBar.setMaximum(100)

    self.value = 0
    self.done = False

    self.manager = QgsAutoGCPManager()

    self.centerDialog()
    self.text = "Loading..."
    
    self.setWindowFlags(Qt.SplashScreen)
    
  def isDone(self):
    return self.done

  def setManager(self, m):
    self.manager = m

  def getProgress(self):
    return self.value

  def progress(self):
    try:
      self.ui.progressBar.setValue(0)
      self.done = False
      self.ui.label.setText(self.text)
      counter = int(0)
      while not self.manager.done():
        prog = int(self.manager.progress() * 100)
        qApp.processEvents()
        if (prog > counter):
          counter = prog
          self.ui.progressBar.setValue(counter)
      self.done = True
    except:
      QgsLogger.debug( "Progressing failed: ", 1, "progressdialog.py", "ProgressDialog::progress()")

  def setText(self, t):
    try:
      self.text = t
      self.ui.label.setText(self.text)
      self.repaint()
    except:
      QgsLogger.debug( "Setting text failed", 1, "progressdialog.py", "ProgressDialog::setText()")

  def setLabel(self, l):
    self.ui.label = l

  def centerDialog(self):
    try:
      screen = QtGui.QApplication.desktop()
      width = self.width()
      height = self.height()
      x = (screen.screenGeometry(screen.primaryScreen()).width() - width) / 2
      y = (screen.screenGeometry(screen.primaryScreen()).height() - height) / 2
      self.move(x, y)
    except:
      QgsLogger.debug( "Centering the dialog failed", 1, "progressdialog.py", "ProgressDialog::centerDialog()")