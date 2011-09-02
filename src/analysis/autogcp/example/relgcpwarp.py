import sys
import os

inputImageFileName = sys.argv[1]
inputGcpsFileName = sys.argv[2]
outputImageFileName = sys.argv[3]

fGcps = open(inputGcpsFileName, 'r')

gcps = []
gcpline = ""


for line in fGcps:
    elements = line.split(" ")
    while elements.__contains__(""):
        elements.remove("")
    gcp = {   "id": elements[0], 
              "p" : elements[1],
              "l" : elements[2],
              "x" : elements[3],
              "y" : elements[4]
          }
    gcps.append(gcp)
    gcpline.__add__(" -gcp %s %s %s %s" % (gcp["p"], gcp["l"], gcp["x"], gcp["y"])) 

warptemp = '/tmp/warpinput.tif'
os.system("gdal_translate -a_srs +proj=longlat %s %s %s" % (gcpline, inputImageFileName, warptemp))
os.system("gdalwarp -order 3 %s %s" % (warptemp, outputImageFileName))

