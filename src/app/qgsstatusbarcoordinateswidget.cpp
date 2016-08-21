/***************************************************************************
   qgsstatusbarcoordinateswidget.cpp
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

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QSpacerItem>
#include <QTimer>
#include <QLabel>
#include <QEvent>
#include <QMouseEvent>
#include "qgsstatusbarcoordinateswidget.h"
#include "qgsapplication.h"
#include "qgsmapcanvas.h"
#include "qgsproject.h"
#include "qgscoordinateutils.h"
#include "qgslogger.h"


/* Since 2.16 this widget will use a label to display info. If you click the label
   it will hide the label and show the input widget. When the widget loses
   focus it will be hidden again and the label will be shown. */
QgsStatusBarCoordinatesWidget::QgsStatusBarCoordinatesWidget( QWidget *parent )
    : QWidget( parent )
    , mDizzyTimer( nullptr )
    , mMapCanvas( nullptr )
    , mMousePrecisionDecimalPlaces( 0 )
{
  mViewMode = QgsStatusBarCoordinatesWidget::Coordinates;

  // a label with icon that acts as a toggle to switch
  // between mouse pos and extents display in status bar widget
  // when clicked
  mToggleExtentsViewLabel = new QLabel( this );
  //mToggleExtentsViewLabel->setMaximumWidth( 20 );
  //mToggleExtentsViewLabel->setBaseSize( 32, 32 );
  mToggleExtentsViewLabel->setFixedSize(16,16);
  mExtentsIcon = QgsApplication::getThemePixmap("mActionToggleExtentsView.svg" );
  mCoordinatesIcon = QgsApplication::getThemePixmap("mActionToggleCoordinatesView.svg" );
  mToggleExtentsViewLabel->setPixmap( mCoordinatesIcon.scaled(
        mToggleExtentsViewLabel->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
  mToggleExtentsViewLabel->setToolTip( tr( "Toggle between extents and mouse position display" ) );

  // add a label to show current position
  mLabel = new QLabel( QString(), this );
  mLabel->setObjectName( "mCoordsLabel" );
  mLabel->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum);
  mLabel->setScaledContents( true );
  mLabel->setAlignment( Qt::AlignCenter );
  mLabel->setFrameStyle( QFrame::NoFrame );
  mLabel->setToolTip( tr( "Current map coordinates" ) );

  mLineEdit = new QLineEdit( this );
  mLineEdit->setMinimumWidth( 10 );
  mLineEdit->setMaximumWidth( 300 );
  mLineEdit->setContentsMargins( 0, 0, 0, 0 );
  mLineEdit->setAlignment( Qt::AlignCenter );
  connect( mLineEdit, SIGNAL( returnPressed() ), this, SLOT( validateCoordinates() ) );

  QRegExp coordValidator( "[+-]?\\d+\\.?\\d*\\s*,\\s*[+-]?\\d+\\.?\\d*" );
  mCoordsEditValidator = new QRegExpValidator( coordValidator, this );
<<<<<<< HEAD

  mLineEdit->setWhatsThis(
        tr( "Shows the map coordinates at the "
            "current cursor position. The display is continuously updated "
            "as the mouse is moved. It also allows editing to set the canvas "
            "center to a given position. The format is longitude,latitude or east,north" ) );
  mLineEdit->setToolTip( tr( "Current map coordinates (longitude,latitude or east,north)" ) );
=======
  mLineEdit->setWhatsThis( tr( "Shows the map coordinates at the "
                               "current cursor position. The display is continuously updated "
                               "as the mouse is moved. It also allows editing to set the canvas "
                               "center to a given position. The format is longitude,latitude or east,north" ) );
  mLineEdit->setToolTip( tr( "Current map coordinate (longitude,latitude or east,north)" ) );

  //toggle to switch between mouse pos and extents display in status bar widget
  mToggleExtentsViewButton = new QToolButton( this );
  mToggleExtentsViewButton->setIcon( QgsApplication::getThemeIcon( "tracking.svg" ) );
  mToggleExtentsViewButton->setToolTip( tr( "Toggle extents and mouse position display" ) );
  mToggleExtentsViewButton->setCheckable( true );
  mToggleExtentsViewButton->setAutoRaise( true );
  connect( mToggleExtentsViewButton, SIGNAL( toggled( bool ) ), this, SLOT( extentsViewToggled( bool ) ) );
>>>>>>> upstream/master

  QHBoxLayout* layout = new QHBoxLayout( this );
  setLayout( layout );
  layout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding ) );
  layout->addWidget( mToggleExtentsViewLabel );
  layout->addWidget( mLabel );
  layout->addWidget( mLineEdit );
  layout->setContentsMargins( 0, 0, 0, 0 );
  layout->setAlignment( Qt::AlignRight );
  layout->setSpacing( 0 );

  // When you feel dizzy
  mDizzyTimer = new QTimer( this );
  connect( mDizzyTimer, SIGNAL( timeout() ), this, SLOT( dizzy() ) );

  // Manage toggle label interactions
  mLineEdit->hide();
  connect( mLineEdit, SIGNAL( editingFinished() ), this, SLOT( showLabel() ) );
  this->setFocusPolicy( Qt::StrongFocus );
}

void QgsStatusBarCoordinatesWidget::setMapCanvas( QgsMapCanvas *mapCanvas )
{
  if ( mMapCanvas )
  {
    disconnect( mMapCanvas, SIGNAL( xyCoordinates( const QgsPoint & ) ), this, SLOT( showMouseCoordinates( const QgsPoint & ) ) );
    disconnect( mMapCanvas, SIGNAL( extentsChanged() ), this, SLOT( showExtent() ) );
  }

  mMapCanvas = mapCanvas;
  connect( mMapCanvas, SIGNAL( xyCoordinates( const QgsPoint & ) ), this, SLOT( showMouseCoordinates( const QgsPoint & ) ) );
  connect( mMapCanvas, SIGNAL( extentsChanged() ), this, SLOT( showExtent() ) );
}

void QgsStatusBarCoordinatesWidget::setFont( const QFont& myFont )
{
  mLineEdit->setFont( myFont );
  mLabel->setFont( myFont );
}

void QgsStatusBarCoordinatesWidget::setMouseCoordinatesPrecision( unsigned int precision )
{
  mMousePrecisionDecimalPlaces = precision;
}


void QgsStatusBarCoordinatesWidget::validateCoordinates()
{
  if ( !mMapCanvas )
  {
    return;
  }
  if ( mLineEdit->text() == "dizzy" )
  {
    // sometimes you may feel a bit dizzy...
    if ( mDizzyTimer->isActive() )
    {
      mDizzyTimer->stop();
      mMapCanvas->setSceneRect( mMapCanvas->viewport()->rect() );
      mMapCanvas->setTransform( QTransform() );
    }
    else
    {
      mDizzyTimer->start( 100 );
    }
    return;
  }
  else if ( mLineEdit->text() == "retro" )
  {
    mMapCanvas->setProperty( "retro", !mMapCanvas->property( "retro" ).toBool() );
    refreshMapCanvas();
    return;
  }

  bool xOk = false;
  bool  yOk = false;
  double x = 0., y = 0.;
  QString coordText = mLineEdit->text();
  coordText.replace( QRegExp( " {2,}" ), " " );

  QStringList parts = coordText.split( ',' );
  if ( parts.size() == 2 )
  {
    x = parts.at( 0 ).toDouble( &xOk );
    y = parts.at( 1 ).toDouble( &yOk );
  }

  if ( !xOk || !yOk )
  {
    parts = coordText.split( ' ' );
    if ( parts.size() == 2 )
    {
      x = parts.at( 0 ).toDouble( &xOk );
      y = parts.at( 1 ).toDouble( &yOk );
    }
  }

  if ( !xOk || !yOk )
    return;

  mMapCanvas->setCenter( QgsPoint( x, y ) );
  mMapCanvas->refresh();
}


void QgsStatusBarCoordinatesWidget::dizzy()
{
  if ( !mMapCanvas )
  {
    return;
  }
  // constants should go to options so that people can customize them to their taste
  int d = 10; // max. translational dizziness offset
  int r = 4;  // max. rotational dizzines angle
  QRectF rect = mMapCanvas->sceneRect();
  if ( rect.x() < -d || rect.x() > d || rect.y() < -d || rect.y() > d )
    return; // do not affect panning
  rect.moveTo(( qrand() % ( 2 * d ) ) - d, ( qrand() % ( 2 * d ) ) - d );
  mMapCanvas->setSceneRect( rect );
  QTransform matrix;
  matrix.rotate(( qrand() % ( 2 * r ) ) - r );
  mMapCanvas->setTransform( matrix );
}

void QgsStatusBarCoordinatesWidget::extentsViewToggled( )
{
  if ( mViewMode == QgsStatusBarCoordinatesWidget::Extents )
  {
    //extents view mode, so switching to coordinates view
    mToggleExtentsViewLabel->setPixmap( mCoordinatesIcon.scaled(
        mToggleExtentsViewLabel->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
    mLineEdit->setReadOnly( true );
    mViewMode = QgsStatusBarCoordinatesWidget::Coordinates;
    mLabel->setToolTip( tr( "Current map coordinates" ) );
    mLabel->setText( tr("Move mouse onto map") );
  }
  else
  {
    //mouse cursor pos view mode so switching to extents
    mToggleExtentsViewLabel->setPixmap( mExtentsIcon.scaled(
        mToggleExtentsViewLabel->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
    mLineEdit->setToolTip( tr( "Map coordinates for the current view extents" ) );
    mLineEdit->setReadOnly( false );
    showExtent();
    mViewMode = QgsStatusBarCoordinatesWidget::Extents;
    mLabel->setToolTip( tr( "Current map extents" ) );
    showExtent();
  }
}

void QgsStatusBarCoordinatesWidget::refreshMapCanvas()
{
  if ( !mMapCanvas )
    return;

  //stop any current rendering
  mMapCanvas->stopRendering();
  mMapCanvas->refreshAllLayers();
}

void QgsStatusBarCoordinatesWidget::showMouseCoordinates( const QgsPoint & p )
{
  if ( mViewMode != QgsStatusBarCoordinatesWidget::Coordinates )
  {
    return;
  }

  // one of these will be hidden depending on whether this widget is focussed
  // so we show position on both - this behaviour added in 2.16
  QString coords = QgsCoordinateUtils::formatCoordinateForProject( p, mMapCanvas->mapSettings().destinationCrs(), mMousePrecisionDecimalPlaces);
  mLabel->setText( coords );
  mLineEdit->setText( coords );

  if ( mLineEdit->width() > mLineEdit->minimumWidth() )
  {
    mLineEdit->setMinimumWidth( mLineEdit->width() );
  }
}


void QgsStatusBarCoordinatesWidget::showExtent()
{
  if ( mViewMode != QgsStatusBarCoordinatesWidget::Extents )
  {
    return;
  }

  // update the statusbar with the current extents.
  QgsRectangle myExtents = mMapCanvas->extent();
  // one of these will be hidden depending on whether this widget is focussed
  // so we show position on both - this behaviour added in 2.16
  mLabel->setText( myExtents.toString( true ) );
  mLineEdit->setText( myExtents.toString( true ) );
  //ensure the label is big enough
  if ( mLineEdit->width() > mLineEdit->minimumWidth() )
  {
    mLineEdit->setMinimumWidth( mLineEdit->width() );
  }
}


void QgsStatusBarCoordinatesWidget::showLabel()
{
  mLineEdit->hide();
  mLabel->show();
}

void QgsStatusBarCoordinatesWidget::mousePressEvent(QMouseEvent* event)
{
  Q_UNUSED(event);
  if ( mToggleExtentsViewLabel->underMouse() ) {
    extentsViewToggled();
  }
  else
  {
    mLineEdit->show();
    mLineEdit->setFocus();
    mLabel->hide();
  }
}
