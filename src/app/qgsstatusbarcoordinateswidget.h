/***************************************************************************
   qgsstatusbarcoordinateswidget.h
    --------------------------------------
   Date                 : 05.08.2015
   Copyright            : (C) 2015 Denis Rouzaud
   Email                : denis.rouzaud@gmail.com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef QGSSTATUSBARCOORDINATESWIDGET_H
#define QGSSTATUSBARCOORDINATESWIDGET_H


class QFont;
class QLabel;
class QLineEdit;
class QTimer;
class QValidator;
class QFocusEvent;

class QgsMapCanvas;
class QgsPoint;

#include <QWidget>

class APP_EXPORT QgsStatusBarCoordinatesWidget : public QWidget
{
    Q_OBJECT

    enum CrsMode
    {
      MapCanvas,
      Custom
    };

    enum ViewMode
    {
      Coordinates,
      Extents
    };

  public:
    QgsStatusBarCoordinatesWidget( QWidget *parent );

    //! define the map canvas associated to the widget
    void setMapCanvas( QgsMapCanvas* mapCanvas );

    void setFont( const QFont& myFont );

    void setMouseCoordinatesPrecision( unsigned int precision );

  signals:
    void coordinatesChanged();

  private slots:
    void showMouseCoordinates( const QgsPoint &p );
    void extentsViewToggled( QMouseEvent* event);
    void validateCoordinates();
    void dizzy();
    void showExtent();
    //! Event handler for when focus is lost so that we can show the label and hide the line edit
    void showLabel();
    //! Event handler for when focus is gained so that we can hide the label and show the line edit
    void mousePressEvent(QMouseEvent* event);

  private:
    void refreshMapCanvas();


    ViewMode mViewMode;
    QLineEdit *mLineEdit;
    QLabel *mToggleExtentsViewLabel;
    //! Widget that will live on the statusbar to display "Coordinate / Extent"
    QLabel *mLabel;

    QValidator *mCoordsEditValidator;
    QTimer *mDizzyTimer;
    QgsMapCanvas* mMapCanvas;

    //! The number of decimal places to use if not automatic
    unsigned int mMousePrecisionDecimalPlaces;

};

#endif // QGSSTATUSBARCOORDINATESWIDGET_H
