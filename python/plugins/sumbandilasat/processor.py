# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui, QtXml
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtXml import *
from qgis.core import *
from qgis.gui import *
from qgis.analysis import *
from ui_processor import Ui_Processor
from settings import SettingsDialog

from distutils import dir_util
from distutils import file_util
import os

from tools.columncorrector.columncorrector import ColumnCorrectorWindow
from tools.linecorrector.linecorrector import LineCorrectorWindow
from tools.bandaligner.bandaligner import BandAlignerWindow
from tools.structurer.structurer import Structurer
from tools.cloudcoverer.cloudcoverer import CloudCovererWindow



class ProcessorDialog(QDialog):

  def __init__(self, iface):
    QDialog.__init__(self)
    self.ui = Ui_Processor()
    self.iface = iface
    self.ui.setupUi(self)
    
    self.cloudWindow = CloudCovererWindow(self.iface)
    QObject.connect(self.cloudWindow, SIGNAL("changed(int, int)"), self.cloudsChanged)
    
    QObject.connect(self.ui.scanButton, SIGNAL("clicked()"), self.scan)
    QObject.connect(self.ui.outputButton, SIGNAL("clicked()"), self.output)
    QObject.connect(self.ui.settingsButton, SIGNAL("clicked()"), self.settings)
    QObject.connect(self.ui.startButton, SIGNAL("clicked()"), self.start)
    QObject.connect(self.ui.stopButton, SIGNAL("clicked()"), self.stop)
    QObject.connect(self.ui.cloudButton, SIGNAL("clicked()"), self.assesClouds)
    QObject.connect(self.ui.removeButton, SIGNAL("clicked()"), self.removeImage)
    
    self.ui.table.horizontalHeader().setResizeMode(2, QHeaderView.Stretch)
    
    self.folders = []
    self.outputFolder = ""
    self.settings = SettingsDialog()
    
    self.mainFiles = []
    self.currentName = ""
    
    self.cloudCover = []
    
    #These 3 variables hold the name of the process that must currently be done, eg: columnCorrection, lineCorrection, etc...
    #and a list of the file paths that need to be processed (input to output)
    #and the index of the current file in the list
    self.currentProcess = ""
    self.currentInputFiles = []
    self.currentOutputFiles = []
    self.currentFile = -1
    self.currentImage = 0
    
    self.totalTasks = 0
    self.processor = None
    self.structurer = Structurer()
    self.dimFile = ""
    self.wasStopped = False
    self.ui.stopButton.hide()
    
    self.timer = QTime()
    self.timerStarted = False
    
    #self.folders.append(QString("/home/cstallmann/ssimages/I03A9"))
    #self.folders.append(QString("/home/cstallmann/ssimages/I03A1"))
    #self.outputFolder = QString("/home/cstallmann/output")
    
  def showMessage(self, string, type = "info"):
    msgbox = QMessageBox(self)
    msgbox.setText(string)
    if type == "info":
      msgbox.setWindowTitle("SumbandilaSat Information")
      msgbox.setIcon(QMessageBox.Information)
    elif type == "error":
      msgbox.setWindowTitle("SumbandilaSat Error")
      msgbox.setIcon(QMessageBox.Critical)
    elif type == "warning":
      msgbox.setWindowTitle("SumbandilaSat Warning")
      msgbox.setIcon(QMessageBox.Warning)
    elif type == "question":
      msgbox.setWindowTitle("SumbandilaSat Question")
      msgbox.setIcon(QMessageBox.Question)
    elif type == "none":
      msgbox.setWindowTitle("SumbandilaSat")
      msgbox.setIcon(QMessageBox.NoIcon)
    msgbox.setModal(True)
    msgbox.exec_()
    
  def removeImage(self):
    if len(self.ui.table.selectedIndexes()) < 1:
      self.showMessage("Please first select an image from the table below.", "warning")
      return
    index = self.ui.table.selectedIndexes()[0].row()
    self.ui.table.removeRow(index)
    self.folders.pop(index)
    self.cloudCover.pop(index)
    
  def assesClouds(self):
    if len(self.ui.table.selectedIndexes()) < 1:
      self.showMessage("Please first select an image from the table below.", "warning")
      return
    filters = ["*.jpg"]
    myDir = QDir(self.ui.table.item(self.ui.table.selectedIndexes()[0].row(), 2).text()+QDir.separator()+"jpeg")
    files = myDir.entryList(filters, QDir.Files)
    if len(files) > 0:
      self.cloudWindow.setImageId(self.ui.table.selectedIndexes()[0].row())
      self.cloudWindow.changeFile(myDir.absolutePath()+QDir.separator()+files[0])
      self.cloudWindow.show()
      self.cloudWindow.reanalyze()
    
  def cloudsChanged(self, value, index):
    self.cloudCover[index] = [self.cloudCover[index][0], value]
    self.ui.table.setItem(index, 1, QTableWidgetItem(str(value)+"%"))
    
  def output(self):
    path = QFileDialog.getExistingDirectory(self, "Select output directory")
    if path != "":
      self.ui.outputLineEdit.setText(path)
      self.outputFolder = path
    
  def scan(self):
    path = QFileDialog.getExistingDirectory(self, "Select parent directory")
    if path != "":
      for i in range(self.ui.table.rowCount()):
	self.ui.table.removeRow(i)
      self.folders = []
      self.cloudCover = []
      if self.ui.singleRadioButton.isChecked():
	self.scanDirectory(path, 0)
      elif self.ui.multiRadioButton.isChecked():
	self.scanDirectory(path, 1)
    
  #pathType: 0 (Scan only this directory), 1 (Also scan children of children)
  def scanDirectory(self, path, pathType):
    myDir = QDir(path)
    if pathType == 0:
      if not myDir.exists():
	self.showMessage("This directory does not exist.", "error")
	return
      dir8Bit = QDir(path+QDir.separator()+"8bit")
      if not dir8Bit.exists():
	self.showMessage("The provided structure does not have an '8bit' folder.", "error")
	return
      dir16Bit = QDir(path+QDir.separator()+"16bit")
      if not dir16Bit.exists():
	self.showMessage("The provided structure does not have a '16bit' folder.", "error")
	return
      filters = ["*.tif"]
      files = dir16Bit.entryList(filters, QDir.Files)
      if len(files) < 3:
	self.showMessage("The provided structure does not have 3 TIF files in the '16bit' folder.", "error")
	return
      filters = ["*.dim"]
      files = dir16Bit.entryList(filters, QDir.Files)
      if len(files) < 3:
	self.showMessage("The provided structure does not have 3 DIMAP files in the '16bit' folder.", "error")
	return
      dirInfo = QDir(path+QDir.separator()+"info")
      if not dirInfo.exists():
	self.showMessage("The provided structure does not have an 'info' folder.", "error")
	return
      dirJpeg = QDir(path+QDir.separator()+"jpeg")
      if not dirJpeg.exists():
	self.showMessage("The provided structure does not have a 'jpeg' folder.", "error")
	return
      self.ui.table.setRowCount(1)
      name = path.right(path.length() - path.lastIndexOf(QString(QDir.separator())) - 1)
      self.ui.table.setItem(0, 0, QTableWidgetItem(name))
      self.ui.table.setItem(0, 1, QTableWidgetItem("--%"))
      self.ui.table.setItem(0, 2, QTableWidgetItem(path))
      self.folders.append(path)
      self.cloudCover.append([name, -1])
    elif pathType == 1:
      allFolders = myDir.entryList(QDir.Dirs | QDir.NoSymLinks)
      for folder in allFolders:
	if folder != "." or folder != "..":
	  folder = path+QDir.separator()+folder
	  dir8Bit = QDir(folder+QDir.separator()+"8bit")
	  dir16Bit = QDir(folder+QDir.separator()+"16bit")
	  dirInfo = QDir(folder+QDir.separator()+"info")
	  dirJpeg = QDir(folder+QDir.separator()+"jpeg")
	  filters = ["*.tif"]
	  files1 = dir16Bit.entryList(filters, QDir.Files)
	  filters = ["*.dim"]
	  files2 = dir16Bit.entryList(filters, QDir.Files)
	  if len(files1) == 3 and len(files2) == 3 and dir8Bit.exists() and dir16Bit.exists() and dirInfo.exists() and dirJpeg.exists():
	    f1 = files1[0].replace("C00", "").replace("C01", "").replace("C02", "")
	    f2 = files1[1].replace("C00", "").replace("C01", "").replace("C02", "")
	    f3 = files1[2].replace("C00", "").replace("C01", "").replace("C02", "")
	    d1 = files2[0].replace("C00", "").replace("C01", "").replace("C02", "")
	    d2 = files2[1].replace("C00", "").replace("C01", "").replace("C02", "")
	    d3 = files2[2].replace("C00", "").replace("C01", "").replace("C02", "")
	    if f1 == f2 and f2 == f3 and d1 == d2 and d2 == d3:
	      self.ui.table.setRowCount(self.ui.table.rowCount() + 1)
	      name = folder.right(folder.length() - folder.lastIndexOf(QString(QDir.separator())) - 1)
	      self.ui.table.setItem(self.ui.table.rowCount() - 1, 0, QTableWidgetItem(name))
	      self.ui.table.setItem(self.ui.table.rowCount() - 1, 1, QTableWidgetItem("--%"))
	      self.ui.table.setItem(self.ui.table.rowCount() - 1, 2, QTableWidgetItem(folder))
	      self.folders.append(folder)
	      self.cloudCover.append([name, -1])
      if self.ui.table.rowCount() == 0:
	self.showMessage("No SumbandilaSat folders can be found.", "error")
      
  def settings(self):
    self.settings.show()
      
  def stop(self):
    self.updateLog("Stopping process ...")
    self.wasStopped = True
    try:
      self.processor.stop()
    except:
      self.updateLog("Please wait while the current task is being completed.")
    self.startProcessing()
      
  def start(self):
    self.wasStopped = False
    self.ui.stopButton.show()
    self.ui.startButton.hide()
    self.processor = None
    self.currentImage = 0
    self.totalTasks = 0
    if self.ui.columnCheckBox.isChecked():
      self.totalTasks += 1
    if self.ui.lineCheckBox.isChecked():
      self.totalTasks += 1
    if self.ui.alignmentCheckBox.isChecked():
      self.totalTasks += 1
      
    self.currentFile = []
    folder = self.folders[self.currentImage]
    self.doBasicFolderProcessing(folder)
      
    myDir = QDir(folder+QDir.separator()+"16bit")
    files = myDir.entryList(["*.tif"], QDir.Files)
    for f in files:
      self.currentName = folder.right(folder.length() - folder.lastIndexOf(QString(QDir.separator())) - 1)
      self.currentInputFiles.append(self.outputFolder+QDir.separator()+self.currentName+QDir.separator()+"16bit"+QDir.separator()+f)
      self.currentOutputFiles.append(self.outputFolder+QDir.separator()+self.currentName+QDir.separator()+"16bit"+QDir.separator()+f+".A.tif")
	
    self.currentProcess = "columnCorrection"
    self.currentFile = 0
    self.finnished = False
    
    self.taskProgress = 0.0
    self.imageProgress = 0.0
    self.totalProgress = 0.0
    self.startProcessing()
  
  def doBasicFolderProcessing(self, path):
    self.mainFiles = []
    separator = QString(QDir.separator())
    name = path.right(path.length() - path.lastIndexOf(separator) - 1)
    parentDir = QDir(self.outputFolder)
    parentDir.mkdir(name)
    myDir = QDir(parentDir.absolutePath()+separator+name)
    myDir.mkdir("16bit")
    filters = ["*.dim"]
    dir16Bit = QDir(path+separator+"16bit")
    files = dir16Bit.entryList(filters, QDir.Files)
    self.structurer = Structurer(self.folders[self.currentImage])
    if len(files) > 0:
      file_util.copy_file(str(dir16Bit.absolutePath()+separator+files[0]), str(self.outputFolder+separator+name+separator+"16bit"+separator+self.structurer.bandAlignedPath(dir16Bit.absolutePath()+separator+files[0])+".dim"))
      self.dimFile = str(dir16Bit.absolutePath()+separator+files[0])
    filters = ["*.tif"]
    files = dir16Bit.entryList(filters, QDir.Files)
    for f in files:
      n = str(self.outputFolder+QDir.separator()+name+QDir.separator()+"16bit"+QDir.separator()+f)
      file_util.copy_file(str(dir16Bit.absolutePath()+separator+f), n)
      self.mainFiles.append(n)
    myDir.mkdir("8bit")
    myDir.mkdir("jpeg")
    myDir.mkdir("info")
    dir_util.copy_tree(str(path+separator+"info"), str(myDir.absolutePath()+separator+"info"))
    
  def progressTasks(self, value):
    if int(value) != int(self.ui.bandProgressBar.value()) and int(value) <= 100:
      if self.currentProcess == "bandAlignment":
	self.ui.bandProgressBar.setValue(value)     
	self.ui.taskProgressBar.setValue(value)
	tasks = self.imageProgress+(value/self.totalTasks)
	self.ui.imageProgressBar.setValue(tasks)
	if self.ui.taskProgressBar.value() == 100:
	  self.imageProgress += (100/self.totalTasks)
	
	total = 100*((self.currentImage)/len(self.folders))
	total = total+(tasks/len(self.folders))
	self.ui.totalProgressBar.setValue(total)
      else:
	self.ui.bandProgressBar.setValue(value)     
	
	task = self.totalProgress+(value/3)
	self.ui.taskProgressBar.setValue(task)
	if value == 100:
	  self.totalProgress += 33
	  if self.totalProgress == 99:
	    self.ui.taskProgressBar.setValue(100)
	    self.totalProgress = 0
	
	tasks = self.imageProgress+(task/self.totalTasks)
	self.ui.imageProgressBar.setValue(tasks)
	if self.ui.taskProgressBar.value() == 100:
	  self.imageProgress += (100/self.totalTasks)
	
	total = 100*((self.currentImage)/len(self.folders))
	total = total+(tasks/len(self.folders))
	self.ui.totalProgressBar.setValue(total)
      
  def progressTasks2(self, v1, v2, v3):
    self.progressTasks(v3)
    
  def progressTasks3(self, v1, v2, v3):
    self.progressTasks(v1)
    
  def updateLog(self, string):
    if string == "<<Finished>>":
      self.startProcessing()
    else:
      self.ui.log.append(string)
    
  def startProcessing(self):
    if self.wasStopped:
      self.ui.startButton.show()
      self.ui.stopButton.hide()
      self.ui.imageProgressBar.setValue(100)
      self.ui.totalProgressBar.setValue(100)
      self.ui.bandProgressBar.setValue(100)
      self.ui.taskProgressBar.setValue(100)
      self.updateLog("Processing was cancled.")
      self.wasStopped = False
      return
    if not self.timerStarted:
      self.timer.start()
      self.timerStarted = True
    if self.currentProcess == "columnCorrection":
      if self.currentFile < 3 and self.ui.columnCheckBox.isChecked():
	self.columnCorrection(self.currentInputFiles[self.currentFile], self.currentOutputFiles[self.currentFile])
	self.currentFile += 1
	return
      else:
	if self.processor != None:
	  self.disconnect(self.processor, SIGNAL("updated(int,QString, int)"), self.progressTasks)
	  self.disconnect(self.processor, SIGNAL("updatedLog(QString)"), self.updateLog)
	  self.processor.stop()
	  while self.processor.isRunning():
	    pass
	  self.processor.releaseMemory()
	  del self.processor
	  self.processor = None
	self.currentProcess = "lineCorrection"
	self.currentFile = 0
	if self.ui.columnCheckBox.isChecked():
	  self.currentInputFiles[0] = self.currentOutputFiles[0]
	  self.currentInputFiles[1] = self.currentOutputFiles[1]
	  self.currentInputFiles[2] = self.currentOutputFiles[2]
	self.currentOutputFiles[0] = QString(self.currentInputFiles[0]).replace("A.tif", "")
	self.currentOutputFiles[1] = QString(self.currentInputFiles[1]).replace("A.tif", "")
	self.currentOutputFiles[2] = QString(self.currentInputFiles[2]).replace("A.tif", "")
	self.currentOutputFiles[0] = self.currentOutputFiles[0]+"B.tif"
	self.currentOutputFiles[1] = self.currentOutputFiles[1]+"B.tif"
	self.currentOutputFiles[2] = self.currentOutputFiles[2]+"B.tif"
    if self.currentProcess == "lineCorrection":
      if self.currentFile < 3 and self.ui.lineCheckBox.isChecked():
	self.lineCorrection(self.currentInputFiles[self.currentFile], self.currentOutputFiles[self.currentFile])
	self.currentFile += 1
	return
      else:
	if self.ui.lineCheckBox.isChecked():
	  self.currentInputFiles[0] = self.currentOutputFiles[1]
	  self.currentInputFiles[1] = self.currentOutputFiles[2]
	  self.currentInputFiles[2] = self.currentOutputFiles[0]
	else:
	  temp = []
	  temp.append(self.currentInputFiles[0])
	  temp.append(self.currentInputFiles[1])
	  temp.append(self.currentInputFiles[2])
	  self.currentInputFiles[0] = temp[1]
	  self.currentInputFiles[1] = temp[2]
	  self.currentInputFiles[2] = temp[0]
	self.structurer = Structurer(self.folders[self.currentImage])
	folder = self.folders[self.currentImage]
	name = folder.right(folder.length() - folder.lastIndexOf(QString(QDir.separator())) - 1)
	self.currentOutputFiles = [self.outputFolder+QDir.separator()+name+QDir.separator()+"16bit"+QDir.separator()+self.structurer.bandAlignedPath(self.dimFile)+".tif"]
	self.currentProcess = "bandAlignment"
	self.currentFile = 0
    if self.currentProcess == "bandAlignment":
      if self.currentFile < 1 and self.ui.alignmentCheckBox.isChecked():
	self.bandAlignment(self.currentInputFiles, self.currentOutputFiles[0])
	self.currentFile += 1
      else:
	self.createFilesFromOutput()
	self.deleteFiles()
	self.saveCloudCover()
	self.currentImage += 1
	if self.currentImage < len(self.folders):
	  self.currentFile = []
	  folder = self.folders[self.currentImage]
	  self.progressTasks(1)
	  self.doBasicFolderProcessing(folder)
	  self.progressTasks(2)
	  myDir = QDir(folder+QDir.separator()+"16bit")
	  files = myDir.entryList(["*.tif"], QDir.Files)
	  for f in files:
	    name = folder.right(folder.length() - folder.lastIndexOf(QString(QDir.separator())) - 1)
	    self.currentInputFiles.append(self.outputFolder+QDir.separator()+name+QDir.separator()+"16bit"+QDir.separator()+f)
	    self.currentOutputFiles.append(self.outputFolder+QDir.separator()+name+QDir.separator()+"16bit"+QDir.separator()+f+".A.tif")
	  self.currentProcess = "columnCorrection"
	  self.currentFile = 0
	  self.ui.imageProgressBar.setValue(100)
	  self.ui.totalProgressBar.setValue(100)
	  self.ui.bandProgressBar.setValue(100)
	  self.ui.taskProgressBar.setValue(100)
	  self.updateLog("All images were processed!")
	  self.timerStarted = False
	  time = QTime()
	  time = time.addMSecs(self.timer.elapsed())
	  self.updateLog("Time elapsed for image: "+time.toString("hh:mm:ss"))
	  self.startProcessing()
    #self.currentFile = 0
    
  def bandAlignment(self, inPaths, outPath):
    self.processor = None
    chipSize = self.settings.value("bandAlignment", "chipSize", 2)
    minGcps = self.settings.value("bandAlignment", "minimumGcps", 2)
    tolerance = self.settings.value("bandAlignment", "refinementTolerance", 3)
    methode = self.settings.value("bandAlignment", "method", 2)
    if methode == 0:
      methode = QgsBandAligner.Disparity
    elif methode == 1:
      methode = QgsBandAligner.Polynomial1
    elif methode == 2:
      methode = QgsBandAligner.Polynomial2
    elif methode == 3:
      methode = QgsBandAligner.Polynomial3
    elif methode == 4:
      methode = QgsBandAligner.Polynomial4
    elif methode == 5:
      methode = QgsBandAligner.Polynomial5
    disparityX = ""
    disparityY = ""
    if self.settings.value("bandAlignment", "createXMap", 1):
      disparityX = outPath+".disparityX"
    if self.settings.value("bandAlignment", "createYMap", 1):
      disparityY = outPath+".disparityY"
    referenceBand = self.settings.value("bandAlignment", "referenceBand", 2)+1
    self.processor = QgsBandAligner(inPaths, outPath, disparityX, disparityY, chipSize, referenceBand, methode, minGcps, tolerance)
    QObject.connect(self.processor, SIGNAL("progressed(double)"), self.progressTasks)
    QObject.connect(self.processor, SIGNAL("logged(QString)"), self.updateLog)
    self.processor.start()
    
  def lineCorrection(self, inPath, outPath):
    if self.ui.lineCheckBox.isChecked():
      self.processor = None
      createMask = self.settings.value("lineCorrection", "createMask", 1)
      maskPath = ""
      if createMask:
	maskPath = outPath+QString(self.settings.value("lineCorrection", "maskExtension", 0)).replace("*", "")
      self.processor = QgsLineCorrector(inPath, outPath, maskPath)
      
      phase1 = QgsLineCorrector.None
      index1 = self.settings.value("lineCorrection", "phase1", 2)
      if index1 == 1:
	phase1 = QgsLineCorrector.ColumnBased
      elif index1 == 2:
	phase1 = QgsLineCorrector.LineBased
	
      phase2 = QgsLineCorrector.None
      index2 = self.settings.value("lineCorrection", "phase2", 2)
      if index2 == 1:
	phase2 = QgsLineCorrector.ColumnBased
      elif index2 == 2:
	phase2 = QgsLineCorrector.LineBased

      self.processor.setPhases(phase1, phase2, self.settings.value("lineCorrection", "recalculate", 1))
      self.processor.setCreateMask(createMask)
      QObject.connect(self.processor, SIGNAL("progressed(double, double, QString)"), self.progressTasks3)
      QObject.connect(self.processor, SIGNAL("logged(QString)"), self.updateLog)
      self.processor.start()
    
  def columnCorrection(self, inPath, outPath):
    if self.ui.columnCheckBox.isChecked():
      self.processor = None
      self.processor = QgsImageProcessor()
      self.connect(self.processor, SIGNAL("updated(int,QString, int)"), self.progressTasks2)
      self.connect(self.processor, SIGNAL("updatedLog(QString)"), self.updateLog)
    
      self.processor.setInputPath(inPath)
      self.processor.setOutputPath(outPath)
      
      self.processor.setInFilePath("")
      
      if self.settings.value("columnCorrection", "exportBadColumns", 1):
	self.processor.setOutFilePath(outPath+QString(self.settings.value("columnCorrection", "badExtension", 0)).replace("*", ""))
      else:
	self.processor.setOutFilePath("")
	
      if self.settings.value("columnCorrection", "createMask", 1):
	self.processor.setCheckPath(outPath+QString(self.settings.value("columnCorrection", "maskExtension", 0)).replace("*", ""))
      else:
	self.processor.setCheckPath("")
	
      self.processor.setThresholds(self.settings.value("columnCorrection", "gainThreshold", 3), self.settings.value("columnCorrection", "biasThreshold", 3))
      
      self.processor.start(-1, -1)
      
  def deleteFiles(self):
    for f in self.mainFiles:
      if os.path.exists(str(f+".A.tif")):
	os.remove(str(f+".A.tif"))
      if os.path.exists(str(f+".B.tif")):
	os.remove(str(f+".B.tif"))
      if os.path.exists(str(f)):
	os.remove(str(f))
      
  def createFilesFromOutput(self):
    if self.settings.value("general", "create8bit", 1):
      name8 =  self.outputFolder+QDir.separator()+self.currentName+QDir.separator()+"8bit"+QDir.separator()+self.structurer.bandAlignedPath(self.dimFile)+".tif"
      name16 =  self.outputFolder+QDir.separator()+self.currentName+QDir.separator()+"16bit"+QDir.separator()+self.structurer.bandAlignedPath(self.dimFile)+".tif"
      p = QProcess()
      p.start("gdal_translate -scale -ot Byte "+name16+" "+name8)
      p.waitForFinished(99999999)
    if self.settings.value("general", "createJpeg", 1):
      nameJpeg =  self.outputFolder+QDir.separator()+self.currentName+QDir.separator()+"jpeg"+QDir.separator()+self.structurer.bandAlignedPath(self.dimFile)+".tif.jpeg"
      name16 =  self.outputFolder+QDir.separator()+self.currentName+QDir.separator()+"16bit"+QDir.separator()+self.structurer.bandAlignedPath(self.dimFile)+".tif"
      p = QProcess()
      p.start("gdal_translate -of JPEG "+name16+" "+nameJpeg)
      p.waitForFinished(99999999)
     
  def saveCloudCover(self):
    folder = self.folders[self.currentImage]
    name = folder.right(folder.length() - folder.lastIndexOf(QString(QDir.separator())) - 1)
    for m in self.cloudCover:
      if QString(m[0]).startsWith(name):
	filters = ["*.dim"]
	path = self.outputFolder+QDir.separator()+m[0]+QDir.separator()+"16bit"
	dir16Bit = QDir(path)
	files = dir16Bit.entryList(filters, QDir.Files)
	self.saveCloudCoverToFile(path+QDir.separator()+files[0], m[1])
	break
     
  def saveCloudCoverToFile(self, path, percentage):
    xmlFile = QFile(path)    
    doc = QDomDocument("DIM")
    #If cloud coverage was not done, leave element empty
    if not xmlFile.exists() or not xmlFile.open(QIODevice.ReadOnly):
      return False
    if not doc.setContent(xmlFile):
      xmlFile.close()
      return ""
    xmlFile.close()
    docElement = doc.documentElement()
    centre = docElement.elementsByTagName("Scene_Source").item(0)
    tag = doc.createElement("cloudCoverPercentage")
    if percentage == -1:
      val = doc.createTextNode("")
    else:
      val = doc.createTextNode(str(percentage))
    tag.appendChild(val)
    centre.appendChild(tag)
    f = open(path, 'w')
    f.write(str(doc.toString()))
    f.close()
    return True
