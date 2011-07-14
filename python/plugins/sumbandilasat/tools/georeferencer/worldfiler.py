#!python
# -*- coding: utf-8 -*-
"""
***************************************************************************************
worldfiler.py - The class for creating the world files.
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

from PyQt4 import QtCore
from PyQt4.QtCore import *
from qgis.core import *
from qgis.gui import *

class WorldFiler:

  #if update is set to true, only the top corner coordinates will updated, else everything is updated
  def __init__(self, filePath, pixelX, rotationY, rotationX, pixelY, coordinateX, coordinateY):
    self.mPixelX = pixelX
    self.mRotationX = rotationX
    self.mRotationY = rotationY
    self.mPixelY = pixelY
    self.mCoordinateX = coordinateX
    self.mCoordinateY = coordinateY
    self.writeTheFile(filePath)
    
  def writeTheFile(self, path):
    pX = QString.number( self.mPixelX, 'f', 10 )
    pY = QString.number( self.mPixelY, 'f', 10 )
    rX = QString.number( self.mRotationX, 'f', 10 )
    rY = QString.number( self.mRotationY, 'f', 10 )
    cX = QString.number( self.mCoordinateX, 'f', 10 )
    cY = QString.number( self.mCoordinateY, 'f', 10 )
    s = pX+"\n"+rY+"\n"+rX+"\n"+pY+"\n"+cX+"\n"+cY
    f = open(path, "w")
    f.write(s)
    f.flush()