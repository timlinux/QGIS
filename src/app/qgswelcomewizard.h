/***************************************************************************
                          qgswelcomewizard.h  -  description
                             -------------------
    begin                : Fri 18 Feb 2011
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
/* $Id:$ */
#ifndef QGSWELCOMEWIZARD_H
#define QGSWELCOMEWIZARD_H

#include "ui_qgswelcomewizardbase.h"

class QgsWelcomeWizard : public QWizard, private Ui::QgsWelcomeWizardBase
{
    Q_OBJECT
  enum { StartPage, ChooseToRegisterPage, RegisterPage, ThanksPage };
  public:
    QgsWelcomeWizard();
    ~QgsWelcomeWizard();

  private:
    void init();

  private slots:

    //! Overloa nextId from wizard base class so we can skip user reg if needed
    int nextId() const;
};

#endif
