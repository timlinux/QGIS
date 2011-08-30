# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *

class HeightPoint():

  def __init__(self, longZ = "", longV = 0, latZ = "", latV = 0):
    self.longZone = longZ
    self.longValue = longV
    self.latZone = latZ
    self.latValue = latV
    
  def nextLong(self):
    if self.longZone == "N" and self.longValue > 0:
      return HeightPoint("N", self.longValue-1)
    elif self.longZone == "N" and self.longValue == 1:
      return HeightPoint("N", 0)
    elif (self.longZone == "N" or self.longZone == "S") and self.longValue == 0:
      return HeightPoint("S", 1)
    elif self.longZone == "S":
      return HeightPoint("S", self.longValue+1)
      
  def nextLat(self):
    if self.latZone == "W" and self.latValue > 0:
      return HeightPoint("", 0, "W", self.latValue-1)
    elif self.latZone == "W" and self.latValue == 1:
      return HeightPoint("", 0, "E", 0)
    elif (self.latZone == "W" or self.latZone == "E") and self.latValue == 0:
      return HeightPoint("", 0, "E", 1)
    elif self.latZone == "E":
      return HeightPoint("", 0, "E", self.latValue+1)
      
  def prevLong(self):
    if self.longZone == "N" and self.longValue >= 0:
      return HeightPoint("N", self.longValue+1)
    elif (self.longZone == "N" or self.longZone == "S") and self.longValue == 0:
      return HeightPoint("N", 0)
    elif self.longZone == "S":
      return HeightPoint("S", self.longValue-1)
      
  def prevLat(self):
    if self.latZone == "W" and self.latValue >= 0:
      return HeightPoint("", 0, "W", self.latValue+1)
    elif (self.latZone == "W" or self.longZone == "E") and self.latValue == 0:
      return HeightPoint("", 0, "W", 1)
    elif self.latZone == "E":
      return HeightPoint("", 0, "E", self.latValue-1)
      
  def longEqual(self, point):
    if self.longZone == point.longZone and self.longValue == point.longValue:
      return True
    return False
    
  def latEqual(self, point):
    if self.latZone == point.latZone and self.latValue == point.latValue:
      return True
    return False
    
  def compareLong(self, point):
    if point.longZone == "N" and self.longZone == "S":
      return "smaller"
    elif self.longZone == "N" and point.longZone == "S":
      return "bigger"
    elif self.longZone == "N" and point.longZone == "N":
      if self.longValue < point.longValue:
	return "smaller"
      elif self.longValue > point.longValue:
	return "bigger"
      else:
	return "equal"
    elif self.longZone == "S" and point.longZone == "S":
      if self.longValue > point.longValue:
	return "smaller"
      elif self.longValue < point.longValue:
	return "bigger"
      else:
	return "equal"
	
  def compareLat(self, point):
    if point.latZone == "W" and self.latZone == "E":
      return "smaller"
    elif self.latZone == "W" and point.latZone == "E":
      return "bigger"
    elif self.latZone == "W" and point.latZone == "W":
      if self.latValue < point.latValue:
	return "smaller"
      elif self.latValue > point.latValue:
	return "bigger"
      else:
	return "equal"
    elif self.latZone == "E" and point.latZone == "E":
      if self.latValue > point.latValue:
	return "smaller"
      elif self.latValue < point.latValue:
	return "bigger"
      else:
	return "equal"
    
  def detectFromPath(self, path):
    path = QString(path)
    index = path.lastIndexOf("/")
    path = path.right(path.length()-index-1)
    self.longZone = path.left(1)
    path = path.right(path.length()-1)
    self.longValue = int(path.left(2))
    path = path.right(path.length()-2)
    self.latZone = path.left(1)
    path = path.right(path.length()-1)
    self.latValue = int(path.left(3))

class HeightCollection():

  def __init__(self, tl, tr, br, bl):
    self.topLeft = tl
    self.topRight = tr
    self.bottomRight = br
    self.bottomLeft = bl

class CoordinateAnalayzer():

  def __init__(self, corners):
    self.corners = [corners.topLeft, corners.topRight, corners.bottomRight, corners.bottomLeft]
    self.biggest = HeightPoint()
    self.smallest = HeightPoint()
    self.allNames = []
    self.orderLongList()
    self.orderLatList()
    self.calculateAll()
    
  def orderLongList(self):
    self.smallest.longZone = self.corners[0].longZone
    self.smallest.longValue = self.corners[0].longValue
    self.biggest.longZone = self.corners[0].longZone
    self.biggest.longValue = self.corners[0].longValue
    for i in range(len(self.corners)):
      if i > 0:
	compareS = self.smallest.compareLong(self.corners[i])
	compareB = self.biggest.compareLong(self.corners[i])
	if compareS == "smaller":
	  self.smallest.longZone = self.corners[i].longZone
	  self.smallest.longValue = self.corners[i].longValue
	if compareB == "bigger":
	  self.biggest.longZone = self.corners[i].longZone
	  self.biggest.longValue = self.corners[i].longValue
	  
  def orderLatList(self):
    self.smallest.latZone = self.corners[0].latZone
    self.smallest.latValue = self.corners[0].latValue
    self.biggest.latZone = self.corners[0].latZone
    self.biggest.latValue = self.corners[0].latValue
    for i in range(len(self.corners)):
      if i > 0:
	compareS = self.smallest.compareLat(self.corners[i])
	compareB = self.biggest.compareLat(self.corners[i])
	if compareS == "smaller":
	  self.smallest.latZone = self.corners[i].latZone
	  self.smallest.latValue = self.corners[i].latValue
	if compareB == "bigger":
	  self.biggest.latZone = self.corners[i].latZone
	  self.biggest.latValue = self.corners[i].latValue
	  
  def pad(self, val, places = 2):
    result = str(val)
    while len(result) < places:
      result = "0"+result
    return result
     
  def calculateAll(self):
    pointLong = self.smallest.prevLong()
    self.smallest.longZone = pointLong.longZone
    self.smallest.longValue = pointLong.longValue
      
    pointLat = self.smallest.prevLat()
    self.smallest.latZone = pointLat.latZone
    self.smallest.latValue = pointLat.latValue

    longs = []
    while True:
      longs.append(self.smallest.longZone+self.pad(self.smallest.longValue, 2))
      pointLong = self.smallest.nextLong()
      self.smallest.longZone = pointLong.longZone
      self.smallest.longValue = pointLong.longValue
      if self.biggest.longEqual(self.smallest):
	longs.append(self.smallest.longZone+self.pad(self.smallest.longValue, 2))
	pointLong = self.smallest.nextLong()
	#Just add another zone to make sure everything is added
	self.smallest.longZone = pointLong.longZone
	self.smallest.longValue = pointLong.longValue
	longs.append(self.smallest.longZone+self.pad(self.smallest.longValue, 2))
	break

    lats = []
    while True:
      lats.append(self.smallest.latZone+self.pad(self.smallest.latValue, 3))
      pointLat = self.smallest.nextLat()
      self.smallest.latZone = pointLat.latZone
      self.smallest.latValue = pointLat.latValue
      if self.biggest.latEqual(self.smallest):
	lats.append(self.smallest.latZone+self.pad(self.smallest.latValue, 3))
	#Just add another zone to make sure everything is added
	pointLat = self.smallest.nextLat()
	self.smallest.latZone = pointLat.latZone
	self.smallest.latValue = pointLat.latValue
	lats.append(self.smallest.latZone+self.pad(self.smallest.latValue, 3))
	break
      
    for along in longs:
      for alat in lats:
	self.allNames.append(along+alat)
      