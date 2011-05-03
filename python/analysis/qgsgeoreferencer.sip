/***************************************************************************
qgsgeoreferencer.sip - Georeferences an image using a set of GCPs. Similar to
 the gdal_translate tool.
--------------------------------------
Date : 20-Oct-2010
Copyright : (C) 2010 by James Meyer
Email : jamesmeyerx@gmail.com

/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgsgeoreferencer.sip 606 2010-10-29 09:13:39Z jamesm $ */

class QgsGeoreferencer
{
%TypeHeaderCode
#include <qgsgeoreferencer.h>
%End
public:
  enum GeorefError {
    ErrorNone = 0,
      ErrorNoGcps = 1,
      ErrorFailed = 2,
      ErrorDriver = 3
  };
  QgsGeoreferencer();

  QString outputFile()const;
  void setOutputFile(const QString& filename);
  QString driver()const;
  void setDriver(const QString& driver);
  QgsGcpSet* gcpSet() const;
  void setGcpSet(QgsGcpSet* set);
  QgsRasterDataset* sourceDataset()const;
  void setSourceDataset(QgsRasterDataset* srcDs);
  GDALDataType outputType()const;
  void setOutputType(GDALDataType type);

  bool applyGeoreference() ;

};

