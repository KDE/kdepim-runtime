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

#include "kconfigwizard.h"

#include <klocale.h>
#include <kdebug.h>
#include <kconfigskeleton.h>

#include <qlistview.h>
#include <qlayout.h>
#include <qtimer.h>

KConfigWizard::KConfigWizard( KConfigPropagator *propagator, QWidget *parent,
                              char *name, bool modal )
  : KDialogBase( TreeList, i18n("Configuration Wizard"), Ok|Cancel, Ok, parent,
                 name, modal ),
    mPropagator( propagator ), mChangesPage( 0 )
{
  connect( this, SIGNAL( aboutToShowPage( QWidget * ) ),
           SLOT( slotAboutToShowPage( QWidget * ) ) );

  QTimer::singleShot( 0, this, SLOT( readConfig() ) );
}

KConfigWizard::~KConfigWizard()
{
  delete mPropagator;
}

void KConfigWizard::slotAboutToShowPage( QWidget *page )
{
  if ( page == mChangesPage ) {
    updateChanges();
  }
}

QFrame *KConfigWizard::createWizardPage( const QString &title )
{
  return addPage( title );
}

void KConfigWizard::setupRulesPage()
{
  QFrame *topFrame = addPage( i18n("Rules") );
  QVBoxLayout *topLayout = new QVBoxLayout( topFrame );
  
  mRuleView = new QListView( topFrame );
  topLayout->addWidget( mRuleView );
  
  mRuleView->addColumn( i18n("Source") );
  mRuleView->addColumn( i18n("Target") );
  mRuleView->addColumn( i18n("Condition") );

  updateRules();
}

void KConfigWizard::updateRules()
{
  mRuleView->clear();

  KConfigPropagator::Rule::List rules = mPropagator->rules();
  KConfigPropagator::Rule::List::ConstIterator it;
  for( it = rules.begin(); it != rules.end(); ++it ) {
    KConfigPropagator::Rule r = *it;
    QString source = r.sourceFile + "/" + r.sourceGroup + "/" +
                     r.sourceEntry;
    QString target = r.targetFile + "/" + r.targetGroup + "/" +
                     r.targetEntry;
    QString condition;
    KConfigPropagator::Condition c = r.condition;
    if ( c.isValid ) {
      condition = c.file + "/" + c.group + "/" + c.key + " = " + c.value;
    }
    new QListViewItem( mRuleView, source, target, condition );
  }
}

void KConfigWizard::setupChangesPage()
{
  QFrame *topFrame = addPage( i18n("Changes") );
  QVBoxLayout *topLayout = new QVBoxLayout( topFrame );
  
  mChangeView = new QListView( topFrame );
  topLayout->addWidget( mChangeView );
  
  mChangeView->addColumn( i18n("Option") );
  mChangeView->addColumn( i18n("Value") );

  mChangesPage = topFrame;
}

void KConfigWizard::updateChanges()
{
  kdDebug() << "KConfigWizard::updateChanges()" << endl;

  usrWriteConfig();

  mChangeView->clear();

  KConfigPropagator::Change::List changes = mPropagator->changes();
  KConfigPropagator::Change::List::ConstIterator it;
  for( it = changes.begin(); it != changes.end(); ++it ) {
    KConfigPropagator::Change c = *it;
    QString option = c.file + "/" + c.group + "/" + c.name;
    QString value = c.value;
    if ( c.hideValue ) value = "*";
    new QListViewItem( mChangeView, option, value );
  }
}

void KConfigWizard::readConfig()
{
  kdDebug() << "KConfigWizard::readConfig()" << endl;

  usrReadConfig();
}

void KConfigWizard::slotOk()
{
  usrWriteConfig();
  mPropagator->skeleton()->writeConfig();
  mPropagator->commit();
  accept();
}

#include "kconfigwizard.moc"
