# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from regressionanalyzer import RegressionAnalyzer
import osgeo.gdal as gdal
from osgeo.gdalconst import *
import struct
from multiprocessing import Process, Lock, Value, Array, Queue
import threading
from numpy import *
from copy import *
from qgis.core import *
from qgis.gui import *

totalTasks = 0
progress = 0.0
progressIncrease = 0.0
processedProgress = 0

totalProgress = 0.0
totalProgressIncrease = 0.0
totalProcessedProgress = 0

bandProgress = 0.0
bandProgressIncrease = 0.0
bandProcessedProgress = 0

class ImageProcessor(threading.Thread):
  
  def __init__(self, parentThread):
    #QThread.__init__( self, parentThread )
    threading.Thread.__init__(self)
    self.running = False
    self.progress1 = Value("i", 0)
    self.progress2 = Value("i", 0)
    self.progress3 = Value("i", 0)
    self.log = Queue()
    self.bad = []
    
    self.inputRasterPath = ""
    self.inputBadPath = ""
    self.outputBadPath = ""
    self.outputRasterPath = ""
    self.outputMaskPath = ""
    
    self.inputBadBool = False
    self.outputBadBool = False
    self.outputRasterBool = False
    self.outputMaskBool = False
    
    self.gainThreshold = 0.0
    self.biasThreshold = 0.0
    self.startColumn = 0
    self.endColumn = 0
    
    self.bandColumns = None
    
  def setTasks(self, inputBad, outputBad, outputRaster, outputMask):
    self.inputBadBool = inputBad
    self.outputBadBool = outputBad
    self.outputRasterBool = outputRaster
    self.outputMaskBool = outputMask
    
  def setTaskValues(self, inputRaster, inputBad, outputBad, outputRaster, outputMask, gain, bias, start, end):
    self.inputRasterPath = inputRaster
    self.inputBadPath = inputBad
    self.outputBadPath = outputBad
    self.outputRasterPath = outputRaster
    self.outputMaskPath = outputMask
    self.gainThreshold = gain
    self.biasThreshold = bias
    self.startColumn = start
    self.endColumn = end

  def readBand(self, band, cols, rows):
    self.log.put("Reading DN values from band")
    data = []
    for row in range(0, rows):
      data.append(band.ReadAsArray(0, row, cols, 1))
    self.createColumns(data, cols, rows)
    self.log.put("All DN values received from band")
    
  def createColumns(self, data, cols, rows):
    self.bandColumns = []
    for col in range(0, cols):
      column = []
      for row in range(0, rows):
	d = data[row]
	column.append(d[0, col])
      self.advanceProgress()
      self.bandColumns.append(column)

  def readColumnFaster(self, col, band, cols, rows):
    return self.bandColumns[col]

  def readColumn(self, col, band, rows):
    result = []
    val = band.ReadRaster(col, 0, 1, rows, 1, rows, gdal.GDT_UInt16)
    data = struct.unpack("H"*rows, val)
    for i in range(rows):
      result.append(data[i])
    return result
  
  def correctColumn(self, band, column, m, c, rows):
    data = struct.unpack("H"*rows, band.ReadRaster(column, 0, 1, rows, 1, rows, gdal.GDT_UInt16))
    dataList = list(data)
    newData = zeros([rows, 1], int)
    for o in range(rows):
      #newData[o, 0] = (data[o]*m+c) - data[o]
      newData[o, 0] = data[o] - ((data[o]*m+c) - data[o])
    self.log.put("Old: "+str(data[2000])+"  New: "+str(newData[2000, 0]))
    band.WriteArray(newData, column, 0)
    band.FlushCache()
    self.log.put("Column " + str(column) + " corrected")
  
  def correctColumns(self, indataset, badRasterColumns, mRasterValues, cRasterValues, path):
    rows = indataset.RasterYSize
    cols = indataset.RasterXSize
    bands = indataset.RasterCount  
    outdataset = indataset.GetDriver().CreateCopy(path, indataset)
    
    totalCols = 0
    for band in range(bands):
      badColumns = badRasterColumns[band]
      totalCols += len(badColumns)
    if totalCols != 0:
      self.startProgress(totalCols/bands, bands)
    
    for band in range(bands):
      outraster = outdataset.GetRasterBand(band+1)
      badColumns = badRasterColumns[band]
      mValues = mRasterValues[band]
      cValues = cRasterValues[band]
      for column in range(len(badColumns)):
	theColumn = badColumns[column]
	self.correctColumn(outraster, theColumn, mValues[column], cValues[column], rows)
	self.advanceProgress()
      
  def readFromFile(self, path, columns, gainValues, biasValues):
    input = QFile(path)
    if input.open(QIODevice.ReadOnly):
      band = 0
      while not input.atEnd():
	band += 1
	line = input.readLine()
	start = line.indexOf(":\t")
	line = line.right(line.length() - (start+1))
	columnsVal = []
	gainValuesVal = []
	biasValuesVal = []
	while line.contains("["):
	  start = line.indexOf("[")
	  end = line.indexOf("]")
	  values = line.mid(start+1, end-(start+1))
	  newStart = values.indexOf("column=")
	  newEnd = values.indexOf("gain=")
	  columnsVal.append(int(values.mid(newStart+7, newEnd-(newStart+8))))
	  newStart = values.indexOf("gain=")
	  newEnd = values.indexOf("bias=")
	  gainValuesVal.append(float(values.mid(newStart+5, newEnd-(newStart+6))))
	  newStart = values.indexOf("bias=")
	  biasValuesVal.append(float(values.right(values.length()-(newStart+5))))
	  line = line.right(line.length() - (end+1))
	columns.append(columnsVal)
	gainValues.append(gainValuesVal)
	biasValues.append(biasValuesVal)
	for i in columnsVal:
	  self.bad.append(str(i)+" (band "+str(band)+")")
      input.close()
      self.log.put("Values loaded from file: " + path)
    else:
      self.log.put("Values could not be loaded from file: " + path)
      
  def saveToFile(self, path, columns, gainValues, biasValues):
    output = QFile(path)
    if output.open(QIODevice.WriteOnly):
      for i in range(len(columns)):
	output.writeData("Band"+str(i)+":\t")
	cValues = columns[i]
	gValues = gainValues[i]
	bValues = biasValues[i]
	for j in range(len(cValues)):
	  output.writeData("[column="+str(cValues[j])+",gain="+str(gValues[j])+",bias="+str(bValues[j])+"]\t")
	output.writeData("\n")
      output.close()
      self.log.put("Values saved to file: " + path)
    else:
      self.log.put("Values could not be saved to file: " + path)
  
  def createCheck(self, originalPath, newPath, checkPath):
    originaldataset = gdal.Open(originalPath, GA_ReadOnly)
    columns = originaldataset.RasterXSize
    rows = originaldataset.RasterYSize
    bands = originaldataset.RasterCount
    newdataset = gdal.Open(newPath, GA_ReadOnly)

    self.startProgress(rows, bands)

    checkdataset = originaldataset.GetDriver().CreateCopy(checkPath, originaldataset)
    self.log.put("Start creating check mask")
    for i in range(1, bands+1):
      self.log.put("Creating band " + str(i) + " for check mask")
      originalband = originaldataset.GetRasterBand(i)
      newband = newdataset.GetRasterBand(i)
      checkband = checkdataset.GetRasterBand(i)
      for j in range(rows):
	originaldata = originalband.ReadAsArray(0, j, columns, 1)
	newdata = newband.ReadAsArray(0, j, columns, 1)
	for k in range(columns):
	  if originaldata[0, k] == newdata[0, k]:
	    originaldata[0, k] = 0
	  else:
	    originaldata[0, k] = 65535
	checkband.WriteArray(originaldata, 0, j)
	self.advanceProgress()
      checkband.FlushCache()
    self.log.put("Check mask created: " + checkPath)
  
  def startProgress(self, devider, bands, tasks = -1):
    global progress
    global progressIncrease
    global processedProgress
    global totalProgress
    global totalProgressIncrease
    global totalProcessedProgress
    global bandProgress
    global bandProgressIncrease
    global bandProcessedProgress
    if tasks != -1:
      global totalTasks
      totalTasks = tasks
    progress = self.getProgress3()
    processedProgress = self.getProgress3()
    progressIncrease= 100.0/(devider*bands*totalTasks)
    totalProgress = 0.0
    totalProgressIncrease= 100.0/(devider*bands)
    totalProcessedProgress = 0
    bandProgress = 0.0
    bandProgressIncrease= 100.0/(devider)
    bandProcessedProgress = 0
    self.progress1.value = 0
    self.progress2.value = 0
  
  def advanceProgress(self):
    global totalProgress
    global totalProcessedProgress
    global progress
    global processedProgress
    global bandProgress
    global bandProcessedProgress
    progress += progressIncrease
    totalProgress += totalProgressIncrease
    bandProgress += bandProgressIncrease
    if int(progress) > processedProgress or int(totalProgress) > totalProcessedProgress or int(bandProgress) > bandProcessedProgress:
      totalProcessedProgress = int(totalProgress)
      processedProgress = int(progress)
      bandProcessedProgress = int(bandProgress)
      if bandProcessedProgress > self.progress1.value:
	self.progress1.value = bandProcessedProgress
      if totalProcessedProgress > self.progress2.value:
	self.progress2.value = totalProcessedProgress
      if processedProgress > self.progress3.value:
	self.progress3.value = processedProgress
      #print "Total progress: " + str(int(processedProgress)) + str("%")+"   Sub progress: " + str(int(bandProcessedProgress)) + str("%")
    
  def getProgress1(self):
    return self.progress1.value
    
  def getProgress2(self):
    return self.progress2.value
    
  def getProgress3(self):
    return self.progress3.value
    
  def getLog(self):
    logMessage = QString("")
    while not self.log.empty():
      logMessage += self.log.get()+"\n--------------------------------------------------------------------------------\n"
    logMessage = logMessage.left(logMessage.length()-1)
    return logMessage
    
  def getBadColumns(self):
    return self.bad
    
  def runFirstRound(self, column, stop, analyzer, inband, cols, rows, faultyColumnsLeft, columnsLeftIndex):
    while column < stop:
      self.advanceProgress()
      #colX = self.readColumn(column, inband, rows)
      #colY = self.readColumn(column+1, inband, rows)
      colX = self.readColumnFaster(column, inband, cols, rows)
      colY = self.readColumnFaster(column+1, inband, cols, rows)
      analyzer.setXY( colX, colY)
      #self.log.put("111: "+str(column)+"  "+str(analyzer.m)+"  "+str(analyzer.c))
      if not analyzer.determineQuality():
	faultyColumnsLeft[columnsLeftIndex.value] = column+1
	columnsLeftIndex.value += 1
	self.log.put("Left-Right Scanner: Potential bad column at position "+str(column)+"\n   (gain="+str(analyzer.m)+" bias="+str(analyzer.c)+")")
      column += 1
    
  def runSecondRound(self, column, stop, analyzer, inband, cols, rows, faultyColumnsRight, m, c, columnsRightIndex):
    while column > stop:
      self.advanceProgress()
      #colX = self.readColumn(column, inband, rows)
      #colY = self.readColumn(column-1, inband, rows)
      colX = self.readColumnFaster(column, inband, cols, rows)
      colY = self.readColumnFaster(column-1, inband, cols, rows)
      analyzer.setXY( colX, colY)
      #self.log.put("222: "+str(column)+"  "+str(analyzer.m)+"  "+str(analyzer.c))
      if not analyzer.determineQuality():
	faultyColumnsRight[columnsRightIndex.value] = column-1
	m[columnsRightIndex.value] = analyzer.m
	c[columnsRightIndex.value] = analyzer.c
	columnsRightIndex.value += 1
	self.log.put("Right-Left Scanner: Potential bad column at position "+str(column)+"\n   (gain="+str(analyzer.m)+" bias="+str(analyzer.c)+")")
      column -= 1
    
  def computeFinalFaults(self, analyzer, inband, rows, faultyColumnsLeftVal, faultyColumnsRightVal, columnsLeftIndex, columnsRightIndex, mRightVal, cRightVal, outColumns, mValues, cValues):
    faultyColumnsLeft = []
    faultyColumnsRight = []
    mRight = []
    cRight = []
    for i in range(columnsLeftIndex.value):
      faultyColumnsLeft.append(faultyColumnsLeftVal[i])
    for i in range(columnsRightIndex.value):
      faultyColumnsRight.append(faultyColumnsRightVal[i])
      mRight.append(mRightVal[i])
      cRight.append(cRightVal[i])
    
    counter = 0
    for i in faultyColumnsRight:
      if i in faultyColumnsLeft:
	m = mRight[counter]
	c = cRight[counter]
	totalChange = 0
	colX = self.readColumn(i, inband, rows)
	colY = self.readColumn(i-1, inband, rows)
	analyzer.setXY( colX, colY)
	analyzer.determineQuality()
	totalChange = 0
	for j in range(rows):
	  if int(colX[j]) != int(colX[j]*analyzer.m+analyzer.c):
	    totalChange += 1
	if totalChange > rows*0.99: #If the pixel changes are not done on more than 99% of the column, the column is consired a good column, and is not adapted
	  if not i in outColumns:
	    outColumns.append(i)
	    mValues.append(m)
	    cValues.append(c)
	    self.log.put("Column at position " + str(i) + " marked as faulty")
	else:
	  self.log.put("Column at position " + str(i) + " marked as correct")
      elif i-2 in faultyColumnsLeft:
	colX = self.readColumn(i-1, inband, rows)
	colY = self.readColumn(i-2, inband, rows)
	analyzer.setXY( colX, colY)
	analyzer.determineQuality()
	totalChange = 0
	for j in range(rows):
	  if int(colX[j]) != int(colX[j]*analyzer.m+analyzer.c):
	    totalChange += 1
	if totalChange > rows*0.99:
	  if not (i-1) in outColumns:
	    outColumns.append(i-1)
	    mValues.append(analyzer.m)
	    cValues.append(analyzer.c)
	    self.log.put("Column at position " + str(i-1) + " marked as faulty")
	else:
	  self.log.put("Column at position " + str(i) + " marked as correct")
	  self.log.put("Column at position " + str(i-2) + " marked as correct")
      counter += 1
      self.advanceProgress()
  
  def run(self):
    dataset = gdal.Open(QString(self.inputRasterPath).toAscii().data(), GA_ReadOnly)
    dataset2 = gdal.Open(QString(self.inputRasterPath).toAscii().data(), GA_ReadOnly)
    outdataset = dataset.GetDriver().CreateCopy(QString(self.outputRasterPath).toAscii().data(), dataset)
    
    columns = dataset.RasterXSize
    rows = dataset.RasterYSize
    bands = dataset.RasterCount
    
    ra = RegressionAnalyzer(self.gainThreshold, self.biasThreshold)
    ra2 = RegressionAnalyzer(self.gainThreshold, self.biasThreshold)
    badRasterColumns = []
    mRasterValues = []
    cRasterValues = []
    
    if bands == 1:
      self.log.put("1 band detected")
    else:
      self.log.put(str(bands) + " bands detected")
     
    taskNumbers = 0
    if self.outputMaskBool:
      taskNumbers += 1
    if self.outputRasterBool:
      taskNumbers += 1
      
    if self.inputBadBool:
      self.readFromFile(QString(self.inputBadPath).toAscii().data(), badRasterColumns, mRasterValues, cRasterValues)
      self.startProgress(1, 1, taskNumbers)
    else:
      self.startProgress(1, 1, taskNumbers+3)
      for band in range(1, bands+1):
	self.log.put("Scanning band " + str(band))
	inband = dataset.GetRasterBand(band)
	inband2 = dataset2.GetRasterBand(band)
	outband = outdataset.GetRasterBand(band)

	self.startProgress(columns, band)
	self.readBand(inband, columns, rows)

	columnsLeftIndex = Value("i", 0)
	columnsRightIndex = Value("i", 0)
	faultyColumnsLeft = Array("i", range(columns))
	faultyColumnsRight = Array("i", range(columns))
	mRight = Array("f", range(columns))
	cRight = Array("f", range(columns))
	column1 = copy(self.startColumn)
	stop1 = copy(self.endColumn)-1
	p1 = Process(target=self.runFirstRound, args=(column1, stop1, ra, inband, columns, rows, faultyColumnsLeft, columnsLeftIndex))
	column2 = copy(self.startColumn)
	stop2 = copy(self.endColumn)-1
	p2 = Process(target=self.runSecondRound, args=(stop2, column2, ra2, inband2, columns, rows, faultyColumnsRight, mRight, cRight, columnsRightIndex))
	self.startProgress((stop1-column1), band)
	p1.start()
	p2.start()
	try:
	  p1.join()
	  p2.join()
	except:
	  p2.join()
	  p1.join()
	self.progress1.value = 100
	self.progress2.value = 100
	
	mValues = []
	cValues = []
	badColumns = []
	self.log.put("Computing final faulty columns")
	self.startProgress(len(faultyColumnsRight), band)
	self.computeFinalFaults(ra, inband, rows, faultyColumnsLeft, faultyColumnsRight, columnsLeftIndex, columnsRightIndex, mRight, cRight, badColumns, mValues, cValues)
	  
	badRasterColumns.append(badColumns)
	for i in badColumns:
	  self.bad.append(str(i)+" (band "+str(band)+")")
	mRasterValues.append(mValues)
	cRasterValues.append(cValues)
    
    if self.outputRasterBool:
      self.correctColumns(dataset, badRasterColumns, mRasterValues, cRasterValues, QString(self.outputRasterPath).toAscii().data())
    if self.outputBadBool:
      self.saveToFile(QString(self.outputBadPath).toAscii().data(), badRasterColumns, mRasterValues, cRasterValues)
    if self.outputMaskBool:
      self.createCheck(QString(self.inputRasterPath).toAscii().data(), QString(self.outputRasterPath).toAscii().data(), QString(self.outputMaskPath).toAscii().data())
      
    self.progress1.value = 100
    self.progress2.value = 100
    self.progress3.value = 100
    self.log.put("Proccess finished!")
