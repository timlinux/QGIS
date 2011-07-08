#!python
# -*- coding: utf-8 -*-
"""
***************************************************************************************
thumbnaillist.py - The thumbnail list dialog for the SumbandilaSat plug-in's georeferencer.
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

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from ui_thumbnaillist import Ui_ThumbnailList

class ThumbnailListDialog(QDialog):

  def __init__(self, parent, settings):
    QDialog.__init__( self, parent )
    self.ui = Ui_ThumbnailList()
    self.ui.setupUi( self )
    self.settings = settings
    
    self.ui.toolbar = QToolBar(self)
    self.ui.toolbar.addAction(self.ui.actionRemove)
    self.ui.widget.layout().addWidget( self.ui.toolbar, 0, Qt.AlignRight )
    
    self.connect(self.ui.buttonBox.button(QDialogButtonBox.Close), SIGNAL("clicked()"), self.close)
    self.connect(self.ui.actionRemove, SIGNAL("triggered()"), self.removeThumbnail)
    
    self.loadThumbnails()
    
  def loadThumbnails(self):
    keys = self.settings.allKeys()
    for item in keys:
      if self.settings.value(item).toString() == "Thumbnail":
	self.ui.listWidget.addItem(item)
	
  def removeThumbnail(self):
    row = self.ui.listWidget.currentRow()
    if row >= 0:
      item = self.ui.listWidget.takeItem(row)
      self.settings.remove(item.text())
      item = None
   