/***************************************************************************
                          qgswelcomewizard.cpp  -  description
                             -------------------
    begin                : Sun Feb 20 2010
    copyright            : (C) 2011 by Tim Sutton
    email                : tim@linfiniti.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include <QSettings>
#include "qgsapplication.h"
#include "qgswelcomewizard.h"
#include "qgschoosetoregisterpage.h"

#ifdef Q_OS_MACX
QgsWelcomeWizard::QgsWelcomeWizard()
    : QWizard( NULL, Qt::WindowSystemMenuHint )  // Modeless dialog with close button only
#else
QgsWelcomeWizard::QgsWelcomeWizard()
    : QWizard( NULL )  // Normal dialog in non Mac-OS
#endif
{
  setupUi( this );
  init();
}

QgsWelcomeWizard::~QgsWelcomeWizard()
{
  QSettings mySettings;
  mySettings.setValue( "/qgis/showWelcomeWizard", 0 );
}

void QgsWelcomeWizard::init()
{
  // set the 60x60 icon pixmap
  QPixmap myLogo( QgsApplication::iconsPath() + "qgis-icon-60x60.png" );
  setPixmap( QWizard::WatermarkPixmap, myLogo );
  setPixmap( QWizard::LogoPixmap, myLogo );
  setPixmap( QWizard::BannerPixmap, myLogo );
  setPixmap( QWizard::BackgroundPixmap , myLogo );

  setPage( StartPage, mStartPage );
  setPage( ChooseToRegisterPage, mChooseToRegisterPage ),
  setPage( RegisterPage, mRegisterPage );
  setPage( ThanksPage, mThanksPage );

  mRegisterPage->setGeometry( mThanksPage->geometry() );

}

int QgsWelcomeWizard::nextId() const
{
  switch ( currentId() ) 
  {
    case StartPage:
        return ChooseToRegisterPage;
    case ChooseToRegisterPage:
      if ( mChooseToRegisterPage->mCbxRegister->isChecked() )
      {
        mRegisterPage->webView->setUrl( "http://users.qgis.org/community-map/create_user_form.html" );
        return RegisterPage;
      }
      else
      {
        return ThanksPage;
      }
    case RegisterPage:
        return ThanksPage;
    default:
      return -1;
  }
}
