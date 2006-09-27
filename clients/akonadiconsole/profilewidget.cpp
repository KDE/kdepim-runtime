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

#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QInputDialog>
#include <QtGui/QPushButton>

#include "libakonadi/components/agentinstanceview.h"
#include "libakonadi/components/profileview.h"
#include "libakonadi/profilemanager.h"

#include "profilewidget.h"

class Dialog : public QDialog
{
  public:
    Dialog( QWidget *parent = 0 )
      : QDialog( parent )
    {
      QVBoxLayout *layout = new QVBoxLayout( this );

      mView = new Akonadi::AgentInstanceView( this );

      QDialogButtonBox *box = new QDialogButtonBox( this );

      layout->addWidget( mView );
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
        mAgentInstance = mView->currentAgentInstance();
      else
        mAgentInstance = QString();

      QDialog::done( result );
    }

    QString agentInstance() const
    {
      return mAgentInstance;
    }

  private:
    Akonadi::AgentInstanceView *mView;
    QString mAgentInstance;
};

ProfileWidget::ProfileWidget( QWidget *parent )
  : QWidget( parent )
{
  mManager = new Akonadi::ProfileManager( this );

  QGridLayout *layout = new QGridLayout( this );

  mView = new Akonadi::ProfileView( this );

  layout->addWidget( mView, 0, 0, 1, 5 );

  QPushButton *button = new QPushButton( "Add...", this );
  connect( button, SIGNAL( clicked() ), this, SLOT( addProfile() ) );
  layout->addWidget( button, 1, 1 );

  button = new QPushButton( "Remove", this );
  connect( button, SIGNAL( clicked() ), this, SLOT( removeProfile() ) );
  layout->addWidget( button, 1, 2 );

  button = new QPushButton( "Add Agent...", this );
  connect( button, SIGNAL( clicked() ), this, SLOT( addAgentInstance() ) );
  layout->addWidget( button, 1, 3 );

  button = new QPushButton( "Remove Agent...", this );
  connect( button, SIGNAL( clicked() ), this, SLOT( removeAgentInstance() ) );
  layout->addWidget( button, 1, 4 );
}

void ProfileWidget::addProfile()
{
  const QString profile = QInputDialog::getText( this, "New Profile", "Profile Name:" );
  if ( !profile.isEmpty() )
    mManager->createProfile( profile );
}

void ProfileWidget::removeProfile()
{
  const QString profile = mView->currentProfile();
  if ( !profile.isEmpty() )
    mManager->removeProfile( profile );
}

void ProfileWidget::addAgentInstance()
{
  Dialog dlg( this );
  if ( dlg.exec() ) {
    mManager->profileAddAgent( mView->currentProfile(), dlg.agentInstance() );
  }
}

void ProfileWidget::removeAgentInstance()
{
  QStringList agents = mManager->profileAgents( mView->currentProfile() );

  bool ok = true;
  const QString agent = QInputDialog::getItem( this, "Select Agent", "Agent:", agents, 0, false, &ok );
  if ( ok )
    mManager->profileRemoveAgent( mView->currentProfile(), agent );
}

#include "profilewidget.moc"
