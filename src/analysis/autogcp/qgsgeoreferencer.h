/***************************************************************************
qgsgeoreferencer.h - Georeferences an image using a set of GCPs. Similar to
 the gdal_translate tool.
--------------------------------------
Date : 20-Oct-2010
Copyright : (C) 2010 by James Meyer
Email : jamesmeyerx@gmail.com

***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsgeoreferencer.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSGEOREFERENCER_H
#define QGSGEOREFERENCER_H
#include <QString>
#include "qgsgcpset.h"
#include "qgsrasterdataset.h"
#include "qgsabstractoperation.h"
/*!\ingroup analysis
 *
 * \brief This class performs Georeferencing of an image based on a set of GCP's
 *
 */
class ANALYSIS_EXPORT QgsGeoreferencer : public QgsAbstractOperation
{
  public:
    typedef enum
    {
      ErrorNone = 0,
      ErrorNoGcps = 1,
      ErrorFailed = 2,
      ErrorDriver = 3
    } GeorefError;
    /*! \brief Constructs a QgsGeoreferencer*/
    QgsGeoreferencer();

    /*! \brief Gets the output filename*/
    QString outputFile()const {return mOutputFile;}
    /*! \brief Sets the output filename*/
    void setOutputFile( const QString& filename ) {mOutputFile = filename;}
    /*! \brief Gets the drivername used to create output rasters*/
    QString driver()const {return mDriver;}
    /*! \brief Sets the driver to use for creating output rasters*/
    void setDriver( const QString& driver ) {mDriver = driver;}
    /*! \brief Gets the QgsGcpSet used to perform the georeferencing*/
    QgsGcpSet* gcpSet() const {return mGcpSet;}
    /*! \brief Sets the QgsGcpSet to use for georeferencing*/
    void setGcpSet( QgsGcpSet* set ) { mGcpSet = set;}
    /*! \brief Gets the source QgsRasterDataset*/
    QgsRasterDataset* sourceDataset()const {return mSrcDs;}
    /*! \brief Sets the source raster to use*/
    void setSourceDataset( QgsRasterDataset* srcDs ) { mSrcDs = srcDs;}
    /*! \brief Gets the current output datatype*/
    GDALDataType outputType()const { return mOutputType;}
    /*! \brief Sets the output datatype to use*/
    void setOutputType( GDALDataType type ) {mOutputType = type;}
    /*! \brief Performs the georeference operation
     *
     * If successful this will generate an image with the specified output parameters
     *
     * \return true on success, false otherwise
     */
    bool applyGeoreference() ;
    /*! \brief Gets the current progress of this georeferenc operation*/
    double progress() const;
    /*! \brief Sets the progress of this operation*/
    void setProgress( double value );
  private:
    static int updateProgress( double dfComplete, const char*, void* pProgressArg );
  private:
    GeorefError mError;
    QString mOutputFile;
    QString mDriver;
    QString mDstCRS;
    QgsGcpSet* mGcpSet;
    QgsRasterDataset* mSrcDs;
    double mProgress;
    GDALDataType mOutputType;

};

#endif  /* QGSGEOREFERENCER_H */

