/***************************************************************************
                              qgsdarwincoredialog.cpp
                              -----------------------
  begin                : September 24, 2010
  copyright            : (C) 2010 by Marco Hugentobler
  email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdarwincoredialog.h"
#include <QFileDialog>
#include <QSettings>

QgsDarwinCoreDialog::QgsDarwinCoreDialog( QWidget * parent, Qt::WindowFlags f ): QDialog( parent, f)
{
  setupUi( this );
  mFromFileButton->setChecked( true );
}

QgsDarwinCoreDialog::~QgsDarwinCoreDialog()
{

}

QString QgsDarwinCoreDialog::datasource() const
{
  if( mFromFileButton->isChecked() )
  {
    return mFileLineEdit->text();
  }
  else if( mFromUrlButton->isChecked() )
  {
    return mUrlLineEdit->text();
  }
  else
  {
    return QString();
  }
}

void QgsDarwinCoreDialog::on_mFilePushButton_clicked()
{
  QSettings s;
  QString dir = s.value("/DarwinCorePlugin/lastFileDir", "" ).toString();
  QString fileName = QFileDialog::getOpenFileName ( 0, tr("Select darwin core file"), dir );
  if( !fileName.isNull() )
  {
    mFileLineEdit->setText( fileName );
    s.setValue( "/DarwinCorePlugin/lastFileDir", QFileInfo( fileName ).absolutePath() );
  }
}

void QgsDarwinCoreDialog::on_mFromFileButton_toggled( bool checked )
{
  mFileLineEdit->setEnabled( true );
  mFilePushButton->setEnabled( true );
  mUrlLineEdit->setEnabled( false );
}

void QgsDarwinCoreDialog::on_mFromUrlButton_toggled( bool checked )
{
  mFileLineEdit->setEnabled( false );
  mFilePushButton->setEnabled( false );
  mUrlLineEdit->setEnabled( true );
}
