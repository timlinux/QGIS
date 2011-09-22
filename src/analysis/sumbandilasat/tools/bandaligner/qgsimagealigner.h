#ifndef __QGSIMAGEALIGNER_H__
#define __QGSIMAGEALIGNER_H__

#include "qgsprogressmonitor.h"

#include <gdal_priv.h>

#include <QString>
#include <QList>

namespace QgsImageAligner 
{
/**
 * This function performs band alignment by copying data from the mBand list to the 
 * output file. It determine the offset between the reference band and each other
 * band by doing a correllation between two chips of size nBlockSize. This offsets
 * is stored in the disparity map file. 
 *
 * The disparity map is used to calculate a disparity X and Y map for each pixel in
 * the output pixel. This X,Y disparity is used to fetch pixel data from the source 
 * band and using a 4x4 bicubic convolution filter determine the output pixel value.
 * 
 * Arguments:
 *  monitor - Progress monitor used to detect cancilation request and progress updates.
 *   
 *  outputPath - Path of the ouput file that will be generated.
 * 
 *  disparityXPath - Path of the temporary file used for the X disparity map. 
 *                   If empty, a temporary file will be created.
 * 
 *  disparityYPath - Path of the temporary file used for the Y disparity map. 
 *                   If empty, a temporary file will be created.
 *
 *  disparityGridPath - Path of the temporary file used for the disparity grid. 
 *                  If empty, a temporary file will be created.
 *  
 *  mBands - A list of GDAL raster bands that will be copied to the output file. 
 *           The order of this list will determine the order of the bands in the 
 *           output file.
 * 
 *  nRefBand - Index of the reference band in the mBands list.
 *
 *  nBlockSize - Size of the area used to determine alignment of the two bands. (Default: 201)
 */
extern void performImageAlignment(QgsProgressMonitor &monitor, QString outputPath, 
    QString disparityXPath, QString disparityYPath, QString disparityGridPath, 
    QList<GDALRasterBand*> &mBands, int nRefBand, int nBlockSize);

} // namespace

#endif /* __QGSIMAGEALIGNER_H__ */