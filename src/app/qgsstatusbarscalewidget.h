/***************************************************************************
                         qgsstatusbarscalewidget.h
    begin                : May 2016
    copyright            : (C) 2016 Denis Rouzaud
    email                : denis.rouzaud@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSSTATUSBARSCALEWIDGET_H
#define QGSSTATUSBARSCALEWIDGET_H

class QFont;
class QHBoxLayout;
class QLabel;
class QPixmap;
class QValidator;

class QgsMapCanvas;
class QgsScaleComboBox;


#include <QWidget>

/**
  * Widget to define scale of the map canvas.
  * @note added in 2.16
  */
class APP_EXPORT QgsStatusBarScaleWidget : public QWidget
{
    Q_OBJECT
  public:
    explicit QgsStatusBarScaleWidget( QgsMapCanvas* canvas, QWidget *parent = 0 );

    /** Destructor */
    virtual ~QgsStatusBarScaleWidget();

    /**
     * @brief setScale set the selected scale from double
     * @param scale
     */
    void setScale( double scale );

    /**
     * @brief isLocked check if the scale should be locked to use magnifier instead of scale to zoom in/out
     * @return True if the scale shall be locked
     */
    bool isLocked() const;

    /** Set the font of the text
      * @param font the font to use
      */
    void setFont( const QFont& font );

  public slots:
    void updateScales( const QStringList &scales = QStringList() );

  private slots:
    void userScale() const;
    //! Added in 2.18
    void lockToggled( bool );
    //! Event handler for when focus is lost so that we can show the label and hide the line edit - added in 2.18
    void showLabel();
    //! Event handler for when focus is gained so that we can hide the label and show the line edit - added in 2.18
    void mousePressEvent(QMouseEvent* event);

  signals:
    void scaleLockChanged( bool );

  private:
    QgsMapCanvas* mMapCanvas;
    QHBoxLayout *mLayout;
    //! Icon used to show that scale is locked so we can zoom without changing scale
    QPixmap mLockIcon;
    //! Icon used to show scale is unlocked so that zooming will change scale
    QPixmap mUnlockIcon;
    //! A label showing the lock icon which will behave like a toggle button when clicked.
    QLabel* mLockLabel;

    //! Widget that will live on the statusbar to display icon
    QLabel* mIconLabel;
    //! Widget that will live on the statusbar to display scale
    QLabel* mScaleLabel;
    //! Widget that will live on the statusbar to change / copy scale value
    QgsScaleComboBox* mScale;
};

#endif // QGSSTATUSBARSCALEWIDGET_H
