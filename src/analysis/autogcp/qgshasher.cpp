/***************************************************************************
qgshasher.cpp - A hash calculator used to generate hashes from strings and
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
/* $Id: qgshasher.cpp 506 2010-10-16 14:02:25Z jamesm $ */
#include "qgshasher.h"

QgsHasher::QgsHasher()
{
} //QgsHasher::QgsHasher()

QString QgsHasher::getStringHash( QString data, HashAlgorithm hash )
{
  QCryptographicHash *hasher;
  if ( hash == Md4 )
  {
    hasher = new QCryptographicHash( QCryptographicHash::Md4 );
  }
  else if ( hash == Sha1 )
  {
    hasher = new QCryptographicHash( QCryptographicHash::Sha1 );
  }
  else
  {
    hasher = new QCryptographicHash( QCryptographicHash::Md5 );
  }
  const char *dataArray;
  QByteArray byteArray = data.toLatin1();
  dataArray = byteArray.data();
  hasher->addData( dataArray, data.length() );

  QByteArray resultArray = hasher->result();
  QString result( resultArray.toHex().constData() );
  return result;
} //QString QgsHasher::getStringHash(QString data, HashAlgorithm hash)

QString QgsHasher::getFileHash( QString data, HashAlgorithm hash, int numberOfBytes )
{
  QCryptographicHash *hasher;
  if ( hash == Md4 )
  {
    hasher = new QCryptographicHash( QCryptographicHash::Md4 );
  }
  else if ( hash == Sha1 )
  {
    hasher = new QCryptographicHash( QCryptographicHash::Sha1 );
  }
  else
  {
    hasher = new QCryptographicHash( QCryptographicHash::Md5 );
  }

  QFile file( data );

  if ( ! file.exists() )
  {
    return "Error: File not found";
  }
  if ( ! file.open( QIODevice::ReadOnly ) )
  {
    return "Error: File could not be opened";
  }
  if ( file.size() < numberOfBytes || numberOfBytes < 0 )
  {
    numberOfBytes = file.size(); //If more bytes or if a negative number of bytes are requested than the size of the file, only the size-number of bytes will be read
  }
  QByteArray byteArray = file.read( numberOfBytes );
  file.close();
  hasher->addData( byteArray );
  QByteArray resultArray = hasher->result();
  QString result( resultArray.toHex().constData() );
  return result;
} //QString QgsHasher::getFileHash(QString data, HashAlgorithm hash, int numberOfBytes)
