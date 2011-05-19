/***************************************************************************
qgsimagechip.cpp - Extension of the QgsRasterDataset that enables in-memory storage
 of the image data
--------------------------------------
Date : 12-September-2010
Author : James Meyer
Copyright : (C) 2010 by C.J. Meyer
Email : jamesmeyerx@gmail.com

***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsimagechip.h 606 2010-10-29 09:13:39Z jamesm $ */

#ifndef QGSIMAGECHIP_H
#define QGSIMAGECHIP_H
#include "qgsrasterdataset.h"
/*! \ingroup analysis
 *
 * Subclass of the QgsRasterDataset that enables in-memory storage
 * of the image data
 */
class ANALYSIS_EXPORT QgsImageChip : public QgsRasterDataset
{
  public:
    /*!
     * \brief Creates an in-memory raster with the given properies
     * \param width The width in pixels of the in-memory raster
     * \param height The height in pixels
     * \param type The data type of the pixels in the dataset
     * \param bands The amount of bands of the new dataset
     * \return Returns the constructed dataset or NULL on failure
     */
    static QgsImageChip* createImageChip( int width, int height, GDALDataType type,  int bands = 3 );
    /*!
     * \brief Creates an in-memory raster with from an existing data buffer
     * The data buffer must have enough space allocated to hold
     * (width * height * bands) elements of the given type.
     * \param width The width in pixels of the in-memory raster
     * \param height The height in pixels
     * \param type The data type of the pixels in the dataset
     * \param bands The amount of bands of the new dataset
     * \param data A pointer to the start of the to be used for the dataset
     * \return Returns the constructed dataset or NULL on failure
     */
    static QgsImageChip* createImage( int width, int height, GDALDataType type, int bands, void* data );
    ~QgsImageChip();
    QgsImageChip( const QgsImageChip& other );
    /*!\brief Returns a pointer to the in-memory data
     */
    void* data() {return mData;}
    /*!\brief Returns the size in bytes of the in-memory data buffer
     *
     */
    long dataSize() {return mDataSize;}
  protected:
  private:
    QgsImageChip( GDALDataset* dataset );
    const QgsImageChip& operator =( const QgsImageChip& other );

    void* mData;
    long mDataSize;

};

#endif //QGSIMAGECHIP_H
