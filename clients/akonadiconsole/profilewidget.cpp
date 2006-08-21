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

#include <QtDBus/QDBusConnection>
#include <QtGui/QGridLayout>
#include <QtGui/QInputDialog>
#include <QtGui/QPushButton>

#include "libakonadi/components/profileview.h"

#include "profilewidget.h"

ProfileWidget::ProfileWidget( QWidget *parent )
  : QWidget( parent )
{
  mManager = new org::kde::Akonadi::AgentManager( "org.kde.Akonadi.AgentManager", "/", QDBusConnection::sessionBus(), this );

  QGridLayout *layout = new QGridLayout( this );

  mView = new PIM::ProfileView( this );

  layout->addWidget( mView, 0, 0, 1, 3 );

  QPushButton *button = new QPushButton( "Add...", this );
  connect( button, SIGNAL( clicked() ), this, SLOT( addProfile() ) );
  layout->addWidget( button, 1, 1 );

  button = new QPushButton( "Remove", this );
  connect( button, SIGNAL( clicked() ), this, SLOT( removeProfile() ) );
  layout->addWidget( button, 1, 2 );
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

#include "profilewidget.moc"
