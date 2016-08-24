/***************************************************************************
                         qgsstatusbarrotationwidget.h
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

#ifndef QGSSTATUSBARROTATIONWIDGET_H
#define QGSSTATUSBARROTATIONWIDGET_H

class QFont;
class QHBoxLayout;
class QLabel;
class QPixmap;
class QgsMapCanvas;
class QDoubleSpinBox;


#include <QWidget>

/**
  * Widget to define rotation of the map canvas.
  * @note added in 2.16
  */
class APP_EXPORT QgsStatusBarRotationWidget : public QWidget
{
    Q_OBJECT
  public:
    explicit QgsStatusBarRotationWidget( QgsMapCanvas* canvas, QWidget *parent = 0 );

    /** Destructor */
    virtual ~QgsStatusBarRotationWidget();

    /** Set the font of the text
      * @param font the font to use
      */
    void setFont( const QFont& font );

  public slots:
    /**
     * @brief setRotation set the selected rotation from double
     * @param rotation
     */
    void setRotation( const double rotation =0 );

  private slots:
    //! Event handler for when focus is lost so that we can show the label and hide the line edit - added in 2.18
    void showLabel();
    //! Event handler for when focus is gained so that we can hide the label and show the line edit - added in 2.18
    void mousePressEvent(QMouseEvent* event);

  private:
    QgsMapCanvas* mMapCanvas;
    QHBoxLayout* mLayout;

    //! Widget that will live on the statusbar to display icon
    QLabel* mIconLabel;
    //! Widget that will live on the statusbar to display rotation
    QLabel* mRotationLabel;
    //! Widget that will live on the statusbar to change / copy rotation value
    QDoubleSpinBox* mRotationSpinBox;
};

#endif // QGSSTATUSBARROTATIONWIDGET_H
