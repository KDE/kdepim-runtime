/*
    This file is part of libkdepim.

    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
#ifndef KCONFIGWIZARD_H
#define KCONFIGWIZARD_H

#include <kconfigpropagator.h>
#include <kdepimmacros.h>
#include <kdialogbase.h>

class QListView;

/**
  @short Configuration wizard base class
*/
class KDE_EXPORT KConfigWizard : public KDialogBase
{
    Q_OBJECT
  public:
    /**
      Create wizard. You have to set a propgator with setPropagator() later.
    */
    KConfigWizard( QWidget *parent = 0, char *name = 0, bool modal = false );
    /**
      Create wizard for given KConfigPropagator. The wizard takes ownership of
      the propagator.
    */
    KConfigWizard( KConfigPropagator *propagator, QWidget *parent = 0,
                   char *name = 0, bool modal = false );
    /**
      Destructor.
    */
    virtual ~KConfigWizard();

    /**
      Set propagator the wizard operates on.
    */
    void setPropagator( KConfigPropagator * );
    /**
      Return propagator the wizard operates on.
    */
    KConfigPropagator *propagator() { return mPropagator; }

    /**
      Create wizard page with given title.
    */
    QFrame *createWizardPage( const QString &title );

    /**
      Use this function to read the configuration from the KConfigSkeleton
      object to the GUI.
    */
    virtual void usrReadConfig() = 0;

    /**
      This function is called when the wizard is finished. You have to save all
      settings from the GUI to the KConfigSkeleton object here, so that the
      KConfigPropagator can take them up from there.
    */
    virtual void usrWriteConfig() = 0;

    /**
      Validates the supplied data. Returns a appropiate error when some data
      is invalid. Return QString::null if all data is valid.
    */
    virtual QString validate() { return QString::null; }

  protected slots:
    void readConfig();

    void slotOk();

    void slotAboutToShowPage( QWidget *page );

  protected:
    void init();

    void setupRulesPage();
    void updateRules();
    void setupChangesPage();
    void updateChanges();

  private:
    KConfigPropagator *mPropagator;

    QListView *mRuleView;
    QListView *mChangeView;

    QWidget *mChangesPage;
};

#endif
