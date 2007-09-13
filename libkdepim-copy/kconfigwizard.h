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
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/
#ifndef KCONFIGWIZARD_H
#define KCONFIGWIZARD_H

#include "kdepim_export.h"
#include <kpagedialog.h>
#include <kvbox.h>

class Q3ListView;
class KPageWidgetItem;

namespace KPIM {

class KConfigPropagator;

/**
  @short Configuration wizard base class
*/
class KDEPIM_EXPORT KConfigWizard : public KPageDialog
{
  Q_OBJECT
  public:
    /**
      Create wizard. You have to set a propgator with setPropagator() later.
    */
    explicit KConfigWizard( QWidget *parent = 0, bool modal = false );

    /**
      Create wizard for given KConfigPropagator. The wizard takes ownership of
      the propagator.
    */
    explicit KConfigWizard( KConfigPropagator *propagator, QWidget *parent = 0,
                            bool modal = false );

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
    QWidget *createWizardPage( const QString &title );

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
      Validates the supplied data. Returns an appropriate error when some data
      is invalid. Return QString() if all data is valid.
    */
    virtual QString validate() { return QString(); }

  protected Q_SLOTS:
    void readConfig();

    void slotOk();

    void slotAboutToShowPage( KPageWidgetItem *, KPageWidgetItem *);

  protected:
    void init();

    void setupRulesPage();
    void updateRules();
    void setupChangesPage();
    void updateChanges();

  private:
    KConfigPropagator *mPropagator;

    Q3ListView *mRuleView;
    Q3ListView *mChangeView;

    KPageWidgetItem *mChangesPage;
};

}

#endif
