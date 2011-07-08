# -*- coding: utf-8 -*-
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *
from ui_helpdialog import Ui_HelpDialog

class HelpDialog(QDialog):

  def __init__(self, parent, version = "1.0.0.0"):
    QDialog.__init__(self, parent)
    self.ui = Ui_HelpDialog()
    self.ui.setupUi(self)
    self.ui.textEdit.setText(self.ui.textEdit.toHtml().replace("-.-.-.-", version))
    self.setHelp()

  def setHelp(self):
    myStyle = QgsApplication.reportStyleSheet()
    helpContents = "<head><style>" + myStyle + "</style></head><body>" + self.helpText() + "</body>"
    self.ui.webView.setHtml(helpContents)
    
  def helpText(self):
    help = """
    <h1>Leechy Help</h1>
    <a href="#data">Data Tab</a><br/>
    <a href="#area">Area Tab</a><br/>
    <a href="#zoom">Zoom Tab</a><br/>
    <a href="#advanced">Advanced Tab</a><br/>
    <a href="#extraction">Extraction Tab</a><br/>
    
    <br/>
    
    <a name="data"><h3>Data Tab</h3></a>
      Specify the input and output data sources.
      <ul>
	  <li>Source data: Select one of the sources that will be used for extracting the tiles.</li>
	  <li>Output image: Specify the location for your final output image.</li>
      </ul>
      
    <a name="area"><h3>Area Tab</h3></a>
      Specify the area that will be extracted from the source.
      <ul>
	  <li>Minimum X: The top-left X-coordinate (Google) of the area top be extracted.</li>
	  <li>Minimum Y: The top-left Y-coordinate (Google) of the area top be extracted.</li>
	  <li>Maximum X: The bottom-right X-coordinate (Google) of the area top be extracted.</li>
	  <li>Maximum Y: The bottom-right Y-coordinate (Google) of the area top be extracted.</li>
      </ul>
      The extent can also be retrieved from the current QGIS map canvas' extent.
      
    <a name="zoom"><h3>Zoom Tab</h3></a>
      Specify the zoom level (resolution) of the image.<br/>Please note that a higher level will result in a better resolution, but may take longer to download and process.
      <ul>
	  <li>Maximum level: The online source will be searched until no more data is available at that zoom level. (Only for satellite and hybrid sources).</li>
	  <li>Manual level: Specify the manual level for extraction.</li>
      </ul>
      
    <a name="advanced"><h3>Advanced Tab</h3></a>
      Specify the thresholds for re-downloading or skipping a tile.
      <ul>
	  <li>Re-download: Any tile with more than a certain percentage of white pixels will be rescheduled for download.</li>
	  <li>Skip: Any tile with more than a certain percentage of white pixels will be considered corrupt and will not be re-downloaded.</li>
	  <li>Overhead: Removes all the image overheads so that the image has the exact specified extent. Else the image might be larger than specified.</li>
	  <li>Proxy: Use the proxy settings specified in the QGIS main options.</li>
      </ul>
      
    <a name="extraction"><h3>Extraction Tab</h3></a>
      The current progress and log messages for the process is shown here.
      <ul>
	  <li>Load iamge: The image will be loaded into QGIS after the processing finished.</li>
      </ul>
    """
    return help