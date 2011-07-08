# -*- coding: utf-8 -*-
import numpy
from numpy import *

class RegressionAnalyzer():
  
  #m Threshold: 1.0 -/+ mThresholdVal
  #c Threshold: 0.0 -/+ cThresholdVal
  def __init__(self, mThresholdVal = 0.1, cThresholdVal = 3.0):
    self.x = []
    self.y = []
    self.m = 0.0
    self.c = 0.0
    self.mThreshold = mThresholdVal
    self.cThreshold = cThresholdVal
    
  def setX(self, xVal):
    self.x = xVal
    
  def setY(self, yVal):
    self.y = yVal
    
  def setXY(self, xVal, yVal):
    self.x = xVal
    self.y = yVal
    
  def solveLeastSquare(self):
    a = []
    for i in range(len(self.x)):
      a.append([self.x[i], 1])
    A = numpy.array(a)
    B = numpy.array(self.y)
    result = numpy.linalg.lstsq(A, B)
    return result
    
  def determineQuality(self):
    #return True if the match is good, return False if match is bad
    result = self.solveLeastSquare()
    self.m = result[0][0]
    self.c = result[0][1]
    
    mGood = False
    cGood = False
    if not self.highVariance(1.0, self.mThreshold, self.m):
      mGood = True
      #print "m: "+str(self.m)
    
    if not self.highVariance(0.0, self.cThreshold, self.c):
      cGood = True
      #print "c: "+str(self.c)
      
    if mGood and cGood:
      return True
    else:
      return False
  
  def highVariance(self, baseValue, threshold, value):
    baseThres = baseValue - threshold
    newVal = value - baseThres
    if newVal < 0 or newVal > (threshold+threshold):
      return True
    else:
      return False
