/***************************************************************************
                         qgsstatusbarscalewidget.cpp
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
#include <QToolButton>
#include <QValidator>

#include "qgsstatusbarscalewidget.h"

#include "qgsmapcanvas.h"
#include "qgsscalecombobox.h"

QgsStatusBarScaleWidget::QgsStatusBarScaleWidget( QgsMapCanvas *canvas, QWidget *parent )
    : QWidget( parent )
    , mMapCanvas( canvas )
{
  // add a label to show scale icon
  mIconLabel = new QLabel( this );
  mIconLabel->setFixedSize(32,16);
  QPixmap scaleIcon = QgsApplication::getThemePixmap("mActionStatusScale.svg" );
  mIconLabel->setPixmap( scaleIcon.scaled(
        mIconLabel->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
  mIconLabel->setToolTip( tr( "Show ond switch map scale" ) );
  mIconLabel->setObjectName( "mScaleIcon" );
  mIconLabel->setMinimumWidth( 10 );
  mIconLabel->setMargin( 3 );
  mIconLabel->setAlignment( Qt::AlignCenter );
  mIconLabel->setFrameStyle( QFrame::NoFrame );

  // add a label to show scale when not changing it
  mScaleLabel = new QLabel( this );
  mIconLabel->setToolTip( tr( "Current map scale" ) );
  mIconLabel->setObjectName( "mScaleLabel" );
  mIconLabel->setMinimumWidth( 10 );
  mIconLabel->setMargin( 3 );
  mIconLabel->setAlignment( Qt::AlignCenter );
  mIconLabel->setFrameStyle( QFrame::NoFrame );

  // combo widget for changing scale
  mScale = new QgsScaleComboBox( this );
  mScale->setObjectName( "mScaleEdit" );
  // seems setFont() change font only for popup not for line edit,
  // so we need to set font for it separately
  mScale->setMinimumWidth( 10 );
  mScale->setContentsMargins( 0, 0, 0, 0 );
  mScale->setWhatsThis( tr( "Displays the current map scale" ) );
  mScale->setToolTip( tr( "Current map scale (formatted as x:y)" ) );

<<<<<<< HEAD
  mLockLabel = new QLabel( this );
  mLockLabel->setFixedSize(16,16);
  mLockIcon = QgsApplication::getThemePixmap("mActionStatusLock.svg" );
  mUnlockIcon = QgsApplication::getThemePixmap("mActionStatusUnlock.svg" );
  mLockLabel->setPixmap( mUnlockIcon.scaled(
        mLockLabel->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
  mLockLabel->setToolTip( tr( "Lock the scale to use magnifier tnd scroll wheel to zoom in or out." ) );
=======
  mLockButton = new QToolButton();
  mLockButton->setIcon( QIcon( QgsApplication::getThemeIcon( "/lockedGray.svg" ) ) );
  mLockButton->setToolTip( tr( "Lock the scale to use magnifier to zoom in or out." ) );
  mLockButton->setCheckable( true );
  mLockButton->setChecked( false );
  mLockButton->setAutoRaise( true );
>>>>>>> upstream/master

  // layout
  mLayout = new QHBoxLayout( this );
  mLayout->addWidget( mIconLabel );
  mLayout->addWidget( mScaleLabel );
  mLayout->addWidget( mScale );
  mLayout->addWidget( mLockLabel );
  mLayout->setContentsMargins( 0, 0, 0, 0 );
  mLayout->setAlignment( Qt::AlignRight );
  mLayout->setSpacing( 0 );

  setLayout( mLayout );

  connect( mScale, SIGNAL( scaleChanged( double ) ), this, SLOT( userScale() ) );

  // Manage toggle label interactions
  mScale->hide();
  connect( mScale->lineEdit(), SIGNAL( editingFinished() ), this, SLOT( showLabel() ) );
  connect( mScale, SIGNAL( indexChanged() ), this, SLOT( showLabel() ) );
  this->setFocusPolicy( Qt::StrongFocus );
  //connect( mLockButton, SIGNAL( toggled( bool ) ), this, SIGNAL( scaleLockChanged( bool ) ) );
  //connect( mLockButton, SIGNAL( toggled( bool ) ), mScale, SLOT( setDisabled( bool ) ) );
}

QgsStatusBarScaleWidget::~QgsStatusBarScaleWidget()
{
}

void QgsStatusBarScaleWidget::lockToggled( bool is_locked )
{
  if (is_locked)
  {
    mLockLabel->setPixmap( mLockIcon );
  }
  else
  {
    mLockLabel->setPixmap( mUnlockIcon );
  }
  emit scaleLockChanged( is_locked );
}

void QgsStatusBarScaleWidget::setScale( double scale )
{
  mScale->blockSignals( true );
  mScale->setScale( scale );
  // we update the label too so that switching between
  // read only label mode and editable widget mode are seamless
  mScaleLabel->setText( mScale->toString( scale ) );
  mScale->blockSignals( false );
}

bool QgsStatusBarScaleWidget::isLocked() const
{
  // return mLockButton->isChecked();
  return false;
}

void QgsStatusBarScaleWidget::setFont( const QFont &font )
{
  mIconLabel->setFont( font );
  mScale->lineEdit()->setFont( font );
}

void QgsStatusBarScaleWidget::updateScales( const QStringList &scales )
{
  mScale->updateScales( scales );
}

void QgsStatusBarScaleWidget::userScale() const
{
  // Why has MapCanvas the scale inverted?
  mMapCanvas->zoomScale( 1.0 / mScale->scale() );
}

void QgsStatusBarScaleWidget::showLabel()
{
  mScale->hide();
  mScaleLabel->show();
}

void QgsStatusBarScaleWidget::mousePressEvent(QMouseEvent* event)
{
  Q_UNUSED(event);
  if ( mLockLabel->underMouse() ) {
    mLockLabel->setPixmap( mLockIcon.scaled(
        mLockLabel->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
  }
  else
  {
    mScale->show();
    mScale->setFocus();
    mScaleLabel->hide();
  }
}
