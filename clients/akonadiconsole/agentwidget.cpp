/*
    This file is part of Akonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "agentwidget.h"

#include <akonadi/agenttypewidget.h>
#include <akonadi/agentinstancewidget.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agentinstancecreatejob.h>

#include <KLocale>

#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QMenu>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

using namespace Akonadi;

class Dialog : public QDialog
{
  public:
    Dialog( QWidget *parent = 0 )
      : QDialog( parent )
    {
      QVBoxLayout *layout = new QVBoxLayout( this );

      mWidget = new Akonadi::AgentTypeWidget( this );

      QDialogButtonBox *box = new QDialogButtonBox( this );

      layout->addWidget( mWidget );
      layout->addWidget( box );

      QPushButton *ok = box->addButton( QDialogButtonBox::Ok );
      connect( ok, SIGNAL( clicked() ), this, SLOT( accept() ) );

      QPushButton *cancel = box->addButton( QDialogButtonBox::Cancel );
      connect( cancel, SIGNAL( clicked() ), this, SLOT( reject() ) );

      resize( 450, 320 );
    }

    virtual void done( int result )
    {
      if ( result == Accepted )
        mAgentType = mWidget->currentAgentType();
      else
        mAgentType = QString();

      QDialog::done( result );
    }

    QString agentType() const
    {
      return mAgentType;
    }

  private:
    Akonadi::AgentTypeWidget *mWidget;
    QString mAgentType;
};

AgentWidget::AgentWidget( QWidget *parent )
  : QWidget( parent )
{
  mManager = new Akonadi::AgentManager( this );

  QGridLayout *layout = new QGridLayout( this );

  mWidget = new Akonadi::AgentInstanceWidget( this );
  connect( mWidget, SIGNAL( doubleClicked( const QString& ) ), SLOT( configureAgent() ) );

  layout->addWidget( mWidget, 0, 0, 1, 6 );

  QPushButton *button = new QPushButton( "Add...", this );
  connect( button, SIGNAL( clicked() ), this, SLOT( addAgent() ) );
  layout->addWidget( button, 1, 1 );

  button = new QPushButton( "Remove", this );
  connect( button, SIGNAL( clicked() ), this, SLOT( removeAgent() ) );
  layout->addWidget( button, 1, 2 );

  button = new QPushButton( "Configure...", this );
  connect( button, SIGNAL( clicked() ), this, SLOT( configureAgent() ) );
  layout->addWidget( button, 1, 3 );

  QMenu *syncMenu = new QMenu( this );
  syncMenu->addAction( i18n("Synchronize All"), this, SLOT(synchronizeAgent()) );
  syncMenu->addAction( i18n("Synchronize Collection Tree"), this, SLOT(synchronizeTree()) );
  button = new QPushButton( "Synchronize", this );
  button->setMenu( syncMenu );
  connect( button, SIGNAL( clicked() ), this, SLOT( synchronizeAgent() ) );
  layout->addWidget( button, 1, 4 );

  button = new QPushButton( "Toggle Online/Offline", this );
  connect( button, SIGNAL(clicked()), SLOT(toggleOnline()) );
  layout->addWidget( button, 1, 5 );
}

void AgentWidget::addAgent()
{
  Dialog dlg( this );
  if ( dlg.exec() ) {
    const QString agentType = dlg.agentType();

    if ( !agentType.isEmpty() ) {
      AgentInstanceCreateJob *job = new AgentInstanceCreateJob( agentType, this );
      job->configure( this );
      job->start(); // TODO: check result
    }
  }
}

void AgentWidget::removeAgent()
{
  const QString agent = mWidget->currentAgentInstance();
  if ( !agent.isEmpty() )
    mManager->removeAgentInstance( agent );
}

void AgentWidget::configureAgent()
{
  const QString agent = mWidget->currentAgentInstance();
  if ( !agent.isEmpty() )
    mManager->agentInstanceConfigure( agent, this );
}

void AgentWidget::synchronizeAgent()
{
  const QString agent = mWidget->currentAgentInstance();
  if ( !agent.isEmpty() )
    mManager->agentInstanceSynchronize( agent );
}

void AgentWidget::toggleOnline()
{
  const QString agent = mWidget->currentAgentInstance();
  if ( !agent.isEmpty() )
    mManager->setAgentInstanceOnline( agent, !mManager->agentInstanceOnline( agent ) );
}

void AgentWidget::synchronizeTree()
{
  const QString agent = mWidget->currentAgentInstance();
  if ( !agent.isEmpty() )
    mManager->agentInstanceSynchronizeCollectionTree( agent );
}

#include "agentwidget.moc"
