/*
    Copyright (c) 2010 Laurent Montel <montel@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "setupkolab.h"
#include "kolabproxyresource.h"
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <QProcess>
#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>

#define IMAP_RESOURCE_IDENTIFIER "akonadi_imap_resource"

SetupKolab::SetupKolab( KolabProxyResource* parentResource,WId parent )
  :KDialog(),
   m_ui(new Ui::SetupKolabView),
   m_parentResource( parentResource )
{
  m_ui->setupUi( mainWidget() );
  initConnection();
  updateCombobox();
}

SetupKolab::~SetupKolab()
{
  delete m_ui;
}

void SetupKolab::initConnection()
{

  connect( m_ui->launchWizard, SIGNAL( clicked() ), this, SLOT( slotLaunchWizard() ) );
  connect( Akonadi::AgentManager::self(), SIGNAL( instanceAdded( const Akonadi::AgentInstance & ) ), this, SLOT( slotInstanceAddedRemoved() ) );
  connect( Akonadi::AgentManager::self(), SIGNAL( instanceRemoved( const Akonadi::AgentInstance & ) ), this, SLOT( slotInstanceAddedRemoved() ) );

}

void SetupKolab::updateCombobox()
{
  bool imapAccountFound = false;
  m_ui->imapAccountComboBox->clear();

  Akonadi::AgentInstance::List relevantInstances;
  foreach ( const Akonadi::AgentInstance &instance, Akonadi::AgentManager::self()->instances() ) {
    if ( instance.identifier().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
      m_ui->imapAccountComboBox->addItem( instance.name() );
      imapAccountFound = true;
    }
  }
  if ( imapAccountFound ) {
    m_ui->stackedWidget->setCurrentIndex( 1 );
  } else {
    m_ui->stackedWidget->setCurrentIndex( 0 );
  }
}

void SetupKolab::slotLaunchWizard()
{
  QStringList lst;
  lst.append( "--assistant" );
  lst.append( "imap" );

  const QString path = KStandardDirs::findExe( QLatin1String("accountwizard" ) );
  if( !QProcess::startDetached( path, lst ) )
    KMessageBox::error( this, i18n( "Could not start the account wizard. "
                                 "Please check your installation." ),
                        i18n( "Unable to start account wizard" ) );


}

void SetupKolab::slotInstanceAddedRemoved()
{
  updateCombobox();
}

#include "setupkolab.moc"
