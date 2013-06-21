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
#include <akonadi/entityhiddenattribute.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/session.h>
#include <Akonadi/CollectionFetchScope>
#include <akonadi/kmime/specialmailcollections.h>
#include <akonadi/kmime/messagestatus.h>
#include <KLocalizedString>
#include <KMime/Message>
#include <KNotification>
#include <KIconLoader>
#include <KIcon>

using namespace Akonadi;

NewMailNotifierAgent::NewMailNotifierAgent( const QString &id )
  : AgentBase( id )
{
  changeRecorder()->setMimeTypeMonitored( KMime::Message::mimeType() );
  changeRecorder()->itemFetchScope().setCacheOnly( true );
  changeRecorder()->itemFetchScope().setFetchModificationTime( false );
  changeRecorder()->fetchCollection( true );
  changeRecorder()->setChangeRecordingEnabled( false );
  changeRecorder()->ignoreSession( Akonadi::Session::defaultSession() );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( Akonadi::CollectionFetchScope::All );


  m_timer.setInterval( 5 * 1000 );
  connect( &m_timer, SIGNAL(timeout()), SLOT(showNotifications()) );
  m_timer.setSingleShot( true );
}

bool NewMailNotifierAgent::excludeSpecialCollection(const Akonadi::Collection &collection) const
{
    SpecialMailCollections::Type type = SpecialMailCollections::self()->specialCollectionType(collection);
    switch(type) {
    case SpecialMailCollections::Invalid: //Not a special collection
    case SpecialMailCollections::Inbox:
        return false;
    default:
        return true;
    }
}

void NewMailNotifierAgent::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  if ( collection.hasAttribute<Akonadi::EntityHiddenAttribute>() )
    return;
  if ( excludeSpecialCollection(collection) ) {
    return; // outbox, sent-mail, trash, drafts or templates.
  }

  Akonadi::MessageStatus status;
  status.setStatusFromFlags( item.flags() );
  if ( status.isRead() || status.isSpam() || status.isIgnored() )
    return;

  if ( !m_timer.isActive() ) {
    m_timer.start();
}

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

  const QPixmap pixmap = KIcon( QLatin1String("kmail") ).pixmap( KIconLoader::SizeSmall, KIconLoader::SizeSmall );
  KNotification::event( QLatin1String("new-email"),
                          texts.join( QLatin1String("<br>") ),
                          pixmap,
                          0,
                          KNotification::CloseOnTimeout,
                          KGlobal::mainComponent());
  //qDebug()<<" NewMailNotifierAgent::showNotifications() component name :"<<KGlobal::mainComponent().componentName();


  m_newMails.clear();
}

AKONADI_AGENT_MAIN( NewMailNotifierAgent )


#include "newmailnotifieragent.moc"
