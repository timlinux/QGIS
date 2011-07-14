# -*- coding: utf-8 -*-
def name():
  return "Leechy"

def description():
  return "Extract images from various online sources such as Google and Yahoo"

versionNumber = "1.0.0.0"
def version():
  return versionNumber
  
def icon():
  return "icon.png"
  
def qgisMinimumVersion():
  return "1.6"
	
def authorName():
  return "Christoph Stallmann"

def classFactory( iface ):
  from leechy import Leechy
  return Leechy(iface, versionNumber)
