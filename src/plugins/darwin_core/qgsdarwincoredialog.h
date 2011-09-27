/***************************************************************************
                              qgsdarwincoredialog.h
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

#ifndef QGSDARWINCOREDIALOG_H
#define QGSDARWINCOREDIALOG_H

#include "ui_qgsdarwincoredialogbase.h"

class QgsDarwinCoreDialog: public QDialog, private Ui::QgsDarwinCoreDialogBase
{
  Q_OBJECT
  public:
    QgsDarwinCoreDialog( QWidget * parent = 0, Qt::WindowFlags f = 0 );
    ~QgsDarwinCoreDialog();

    //returns selected datasource (could be a local file path or an url)
    QString datasource() const;

  private slots:
    void on_mFilePushButton_clicked();
    void on_mFromFileButton_toggled( bool checked );
    void on_mFromUrlButton_toggled( bool checked );
};

#endif // QGSDARWINCOREDIALOG_H
