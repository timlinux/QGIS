/***************************************************************************
                         qgsstatusbarrotationwidget.cpp
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

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QValidator>

#include "qgsstatusbarrotationwidget.h"
#include "qgsmapcanvas.h"

QgsStatusBarRotationWidget::QgsStatusBarRotationWidget(
  QgsMapCanvas *canvas, QWidget *parent )
    : QWidget( parent )
    , mMapCanvas( canvas )
{
  // add a label to show rotation icon
  mIconLabel = new QLabel( this );
  mIconLabel->setFixedSize(16,16);
  QPixmap rotationIcon = QgsApplication::getThemePixmap("mActionStatusRotate.svg" );
  mIconLabel->setPixmap( rotationIcon.scaled(
        mIconLabel->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
  mIconLabel->setToolTip( tr( "Show ond adjust map rotation" ) );
  mIconLabel->setObjectName( "mRotationIcon" );
  mIconLabel->setMinimumWidth( 10 );
  mIconLabel->setMargin( 3 );
  mIconLabel->setAlignment( Qt::AlignCenter );
  mIconLabel->setFrameStyle( QFrame::NoFrame );

  // add a widget to set current rotation
  // when it loses focus it will be hidden and
  // the label shown in its place
  mRotationSpinBox = new QDoubleSpinBox( parent );
  mRotationSpinBox->setObjectName( "mRotationEdit" );
  mRotationSpinBox->setMinimumWidth( 10 );
  mRotationSpinBox->setContentsMargins( 0, 0, 0, 0 );
  mRotationSpinBox->setKeyboardTracking( false );
  mRotationSpinBox->setMaximumWidth( 120 );
  mRotationSpinBox->setDecimals( 1 );
  mRotationSpinBox->setRange( -180.0, 180.0 );
  mRotationSpinBox->setWrapping( true );
  mRotationSpinBox->setSingleStep( 5.0 );
  mRotationSpinBox->setWhatsThis( tr(
    "Shows the current map clockwise rotation "
    "in degrees. Changing the value here will alter "
    "the rotation of the map display." ) );
  mRotationSpinBox->setToolTip( tr(
    "Current clockwise map rotation in degrees" ) );
  connect(
    mRotationSpinBox, SIGNAL( editingFinished() ),
    this, SLOT( showRotationLabel() ) );
  connect(
    mRotationSpinBox, SIGNAL( valueChanged( double ) ),
    this, SLOT( userRotation() ) );

  // Add the widget to show the current rotation
  // this will be hidden when you click it and the
  // spin box shown in its place
  mRotationLabel = new QLabel( this );
  mRotationLabel->setObjectName( "mRotationLabel" );
  mRotationLabel->setToolTip( tr(
    "Current clockwise map rotation in degrees" ) );
  // Hide spin box, show label
  showLabel();

  mRotationSpinBox->setWhatsThis( tr(
    "Displays the current map rotation" ) );
  mRotationSpinBox->setToolTip( tr(
    "Current map rotation (formatted as x:y)" ) );

  // layout
  mLayout = new QHBoxLayout( this );
  mLayout->addWidget( mIconLabel );
  mLayout->addWidget( mRotationLabel );
  mLayout->addWidget( mRotationSpinBox );
  mLayout->setContentsMargins( 0, 0, 0, 0 );
  mLayout->setAlignment( Qt::AlignRight );
  mLayout->setSpacing( 0 );
  setLayout( mLayout );

  // Manage toggle label interactions
  mRotationSpinBox->hide();
  connect( mRotationSpinBox, SIGNAL( editingFinished() ), this, SLOT( showLabel() ) );
  connect( mRotationSpinBox, SIGNAL( indexChanged() ), this, SLOT( showLabel() ) );
  this->setFocusPolicy( Qt::StrongFocus );

  // Make sure the label and the spinbox update if the mapcanvas
  // changes its rotation setting
  connect( mMapCanvas, SIGNAL( rotationChanged( double ) ),
           this, SLOT( showRotation() ) );

  this->setRotation(0);
}

QgsStatusBarRotationWidget::~QgsStatusBarRotationWidget()
{
}

void QgsStatusBarRotationWidget::setFont( const QFont &font )
{
  mIconLabel->setFont( font );
  mRotationSpinBox->setFont( font );
}

void QgsStatusBarRotationWidget::setRotation( double rotation )
{
  mRotationSpinBox->blockSignals( true );
  mRotationSpinBox->setValue( rotation );
  // we update the label too so that switching between
  // read only label mode and editable widget mode are seamless
  mRotationLabel->setText(
    QString( "%1").arg(mRotationSpinBox->value()) );
  // Why has MapCanvas the rotation inverted?
  mMapCanvas->setRotation( 1.0 / mRotationSpinBox->value() );
  // listen for changes again
  mRotationSpinBox->blockSignals( false );
}

void QgsStatusBarRotationWidget::showLabel()
{
  mRotationSpinBox->hide();
  mRotationLabel->show();
}

void QgsStatusBarRotationWidget::mousePressEvent(QMouseEvent* event)
{
  Q_UNUSED(event);
  mRotationSpinBox->show();
  mRotationSpinBox->setFocus();
  mRotationLabel->hide();
}
