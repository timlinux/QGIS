#!python
# -*- coding: utf-8 -*-
"""
/***************************************************************************
aboutdialogbase.py - The class providing the graphical user interface for
the AutoGCP plugin's About dialog. All the necessary contact information
to get in touch with the developers of this plugin is provided.
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
from ui_aboutdialogbase import Ui_AboutDialog

class AboutDialog(QDialog):
  
  def __init__(self, parent):
    QDialog.__init__( self, parent )
    self.ui = Ui_AboutDialog()
    self.ui.setupUi( self )
    QObject.connect(self.ui.buttonBox, SIGNAL("rejected()"), self.close)