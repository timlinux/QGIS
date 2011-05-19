/***************************************************************************
qgshasher.h - A hash calculator used to generate hashes from strings and
files.
--------------------------------------
Author: Christoph Stallmann
Date : 24-June-2010
Date Modified: 08-July-2010
Copyright : (C) 2010 by FoxHat Solutions
Email : foxhat.solutions@gmail.com
***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/
/* $Id: qgshasher.h 506 2010-10-16 14:02:25Z jamesm $ */
#ifndef QGSHASHER_H
#define QGSHASHER_H

#include <QString>
#include <QFile>
#include <QByteArray>
#include <QCryptographicHash>

/*! \ingroup analysis
 * A hash calculator used to generate hashes from strings and files.
 */
class ANALYSIS_EXPORT QgsHasher
{
  public:

    /**
    * Options for the hash algorithm to be used.
    **/
    enum HashAlgorithm
    {
      Md4,
      Md5,
      Sha1
    };

    /**
    * Default constructor for the class.
    **/
    QgsHasher();

    /**
    * Calculates the hash for a string.
    *
    * @param data - The string used for the hash calculation.
    * @param hash - Specifies the hash algorithm. Md5 is given as default hash algorithm.
    *
    * @return A string containing the hash.
    *
    **/
    QString getStringHash( QString data, HashAlgorithm hash = Md5 );

    /**
    * Calculates the hash for a file.
    *
    * @param data - The path of the file to be used for the hash calculation.
    * @param hash - Specifies the hash algorithm. Md5 is given as default hash algorithm.
    * @param numberOfBytes - Specefies the number of bytes to be read from the file, used for the hash calculation. -1 is given as default and used to read the whole file.
    *
    * @return A string containing the hash
    *
    **/
    QString getFileHash( QString data, HashAlgorithm hash = Md5, int numberOfBytes = -1 );
};

#endif // QGSHASHER_H
