/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "newmailnotifieragent.h"

#include <akonadi/agentfactory.h>
#include <akonadi/changerecorder.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kmime/messagestatus.h>
#include <KLocalizedString>
#include <KMime/Message>
#include <KNotification>

NewMailNotifierAgent::NewMailNotifierAgent( const QString &id )
  : AgentBase( id )
{
  changeRecorder()->setMimeTypeMonitored( KMime::Message::mimeType() );
  changeRecorder()->itemFetchScope().setCacheOnly( true );
  changeRecorder()->itemFetchScope().setFetchModificationTime( false );
  changeRecorder()->fetchCollection( true );
  changeRecorder()->setChangeRecordingEnabled( false );

  m_timer.setInterval( 30 * 1000 );
  m_timer.setSingleShot( true );
  connect( &m_timer, SIGNAL( timeout() ), SLOT( showNotifications() ) );
}

void NewMailNotifierAgent::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  Akonadi::MessageStatus status;
  status.setStatusFromFlags( item.flags() );
  if ( status.isRead() )
    return;

  if ( !m_timer.isActive() )
    m_timer.start();

  m_newMails[collection]++;
}

void NewMailNotifierAgent::showNotifications()
{
  QStringList texts;
  for ( QHash< Akonadi::Collection, int >::const_iterator it = m_newMails.constBegin(); it != m_newMails.constEnd(); ++it ) {
    Akonadi::EntityDisplayAttribute *attr = it.key().attribute<Akonadi::EntityDisplayAttribute>();
    QString displayName;
    if ( attr && !attr->displayName().isEmpty() )
      displayName = attr->displayName();
    else
      displayName = it.key().name();
    texts.append( i18np( "One new email in %2", "%1 new emails in %2", it.value(), displayName ) );
  }

  kDebug() << texts;
  KNotification *notify = new KNotification( "new-email", 0L, KNotification::Persistent );
  notify->setText( texts.join( "\n" ) );
  notify->sendEvent();

  m_newMails.clear();
}

AKONADI_AGENT_FACTORY( NewMailNotifierAgent, newmailnotifieragent );

#include "newmailnotifieragent.moc"
