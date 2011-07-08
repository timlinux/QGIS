# -*- coding: utf-8 -*-

from PyQt4 import QtCore, QtGui, QtXml
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtXml import *
import math

"""
  Output:
  	ZA2_MSS_R3B_FMC4_024W_34_054N_51_100924_192317_L1Ab_ORBIT-


  SAT_SEN_TYP_MOD_KKKK_KS_JJJJ_JS_YYMMDD_HHMMSS_LEVL      

       Where:
       SAT    Satellite or mission          mandatory
       SEN    Sensor                        mandatory
       TYP    Type                          mandatory?
       MOD    Acquisition mode              mandatory?
       KKKK   Orbit path reference          optional?
       KS     Path shift                    optional?
       JJJJ   Orbit row reference           optional?
       JS     Row shift                     optional?
       YYMMDD Acquisition date              mandatory
       HHMMSS Scene centre acquisition time mandatory
       LEVL   Processing level              mandatory
       PROJTN Projection                    mandatory
"""

class Structurer():
  
  #Path is the path to the original root folder where the folders 16bit, 8bit, info and jpeg are located in
  def __init__(self, path = ""):
    path = QString(path)
    if path.endsWith("/") or path.endsWith("\\"):
      path = path.left(path.length()-1)
    self.path = path
    """self.newPath = self.path + "_Processed" + QDir.separator()
    theDir = QDir("")
    theDir.mkpath(self.newPath + "jpeg")
    theDir.mkpath(self.newPath + "info")
    theDir.mkpath(self.newPath + "16bit")
    theDir.mkpath(self.newPath + "8bit")"""
    
  #Copy the old data that will not change
  def copyOldData(self):
    theDir = QDir(self.path + QDir.separator() + "8bit")
    files = theDir.entryInfoList()
    for f in files:
      copyfile(f.absolutePath(), self.newPath + "8bit")
      
    theDir = QDir(self.path + QDir.separator() + "info")
    files = theDir.entryInfoList()
    for f in files:
      copyfile(f.absolutePath(), self.newPath + "info")
    
  def bandAlignedPath(self, dimFile):
    doc = QDomDocument("DIM")
    xmlFile = QFile(dimFile)    
    if not xmlFile.exists() or not xmlFile.open(QIODevice.ReadOnly):
      return ""
    if not doc.setContent(xmlFile):
      xmlFile.close()
      return ""
    xmlFile.close()
    docElement = doc.documentElement()
    centre = docElement.elementsByTagName("Scene_Center").item(0)
    lat = float(centre.firstChildElement("FRAME_LON").text())
    lon = float(centre.firstChildElement("FRAME_LAT").text())
    dateTime = docElement.elementsByTagName("Production").item(0).firstChildElement("DATASET_PRODUCTION_DATE").text()
    date = dateTime.left(10)
    time = dateTime.right(8)
    return self.sacProductId(lat, lon, date, time)
    
  def sacProductId(self, latitude, longitude, date, time):
    latZone = "N"
    if latitude < 0:
      latZone = "S"
      latitude *= -1
    latDegree = math.floor(latitude)
    latTime = (latitude - latDegree) * 60.0
    latMinute = math.floor(latTime)
    latSecond = math.floor((latTime - latMinute) * 60.0)

    latString = ""
    if latDegree < 99:
      latString += "0"
    if latDegree < 9:
      latString += "0"
    latString += str(int(latDegree)) + latZone + "_" + str(int(latMinute))

    lonZone = "E"
    if longitude < 0:
      lonZone = "W"
      longitude *= -1
    lonDegree = math.floor(longitude)
    lonTime = (longitude - lonDegree) * 60.0
    lonMinute = math.floor(lonTime)
    lonSecond = math.floor((lonTime - lonMinute) * 60.0)

    lonString = ""
    if lonDegree < 99:
      lonString += "0"
    if lonDegree < 9:
      lonString += "0"
    lonString += str(int(lonDegree)) + lonZone + "_" + str(int(lonMinute))
    
    date = QString(date)
    date = date.right(date.length()-2).replace("-", "")
    time = QString(time).replace(":", "")
    return "ZA2_MSI_R3B_FMC4_" + lonString + "_" + latString + "_" + date + "_" + time + "_L1Ab_ORBIT-"