# -*- coding: utf-8 -*-
def name():
  return "Elevator"

def description():
  return "Extract elevation data from an online source"

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
  from elevator import Elevator
  return Elevator(iface, versionNumber)
