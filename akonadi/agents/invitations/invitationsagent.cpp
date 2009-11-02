/*
    Copyright 2009 Sebastian Sauer <sebsauer@kdab.net>

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

#include "invitationsagent.h"
#include "invitationsagent.moc"

#include <kcal/event.h>
#include <kcal/todo.h>
#include <kcal/journal.h>
#include <kcal/kcalmimetypevisitor.h>

#include <KMime/Message>

#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>

AKONADI_AGENT_MAIN( InvitationsAgent )

using namespace Akonadi;

class InvitationsAgent::Private
{
  public:
    Private( InvitationsAgent *q )
        : q( q )
    {
    }

    ~Private()
    {
    }

    InvitationsAgent * const q;
};

InvitationsAgent::InvitationsAgent( const QString &id )
  : Akonadi::AgentBase( id )
  , Akonadi::AgentBase::ObserverV2()
  , d( new Private( this ) )
{
  kDebug() << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";

  changeRecorder()->setChangeRecordingEnabled( false ); // behave like Akonadi::Monitor
  changeRecorder()->itemFetchScope().fetchFullPayload(); //FIXME don't fetch all!
  changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::eventMimeType() );
  changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::todoMimeType() );
  changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::journalMimeType() );
  changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::freeBusyMimeType() );
  changeRecorder()->setMimeTypeMonitored( "text/calendar", true );
  changeRecorder()->setMimeTypeMonitored( "message/rfc822", true );
  //changeRecorder()->setCollectionMonitored( Collection::root(), true );

  //setNeedsNetwork( true );
  //setOnline( true );
  emit status( AgentBase::Idle, "WE ARE RUNNING!!!!" );
}

InvitationsAgent::~InvitationsAgent()
{
  kDebug() << "!!!!!!!!!!!!!!!!!!!";
  delete d;
}

void InvitationsAgent::configure( WId windowId )
{
  Q_UNUSED(windowId);
  kDebug() << "!!!!!!!!!!!!!!!!!!!";
}

void InvitationsAgent::doSetOnline( bool online )
{
  kDebug() << online;
}

void InvitationsAgent::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  kDebug()<<"!!!!!!!!!!!!!!!!!!!"<<item.id()<<item.remoteId();
  if( item.hasPayload<KCal::Event::Ptr>() )
    kDebug()<<"Event";
  else if( item.hasPayload<KCal::Todo::Ptr>() )
    kDebug()<<"Todo";
  else if( item.hasPayload<KCal::Journal::Ptr>() )
    kDebug()<<"Journal";
  else if( item.hasPayload<KMime::Message::Ptr>() )
    kDebug()<<"Message";
  else
    kDebug()<<"NOPE";
}

void InvitationsAgent::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers )
{
  //kDebug()<<"!!!!!!!!!!!!!!!!!!!";
}

void InvitationsAgent::itemRemoved( const Akonadi::Item &item )
{
  //kDebug()<<"!!!!!!!!!!!!!!!!!!!";
}

void InvitationsAgent::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  //kDebug()<<"!!!!!!!!!!!!!!!!!!!";
}

void InvitationsAgent::collectionChanged( const Akonadi::Collection &collection )
{
  //kDebug()<<"!!!!!!!!!!!!!!!!!!!";
}

void InvitationsAgent::collectionRemoved( const Akonadi::Collection &collection )
{
  //kDebug()<<"!!!!!!!!!!!!!!!!!!!";
}

void InvitationsAgent::itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination )
{
  //kDebug()<<"!!!!!!!!!!!!!!!!!!!";
}

void InvitationsAgent::itemLinked( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  //kDebug()<<"!!!!!!!!!!!!!!!!!!!";
}

void InvitationsAgent::itemUnlinked( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  //kDebug()<<"!!!!!!!!!!!!!!!!!!!";
}

void InvitationsAgent::collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination )
{
  //kDebug()<<"!!!!!!!!!!!!!!!!!!!";
}

void InvitationsAgent::collectionChanged( const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes )
{
  //kDebug()<<"!!!!!!!!!!!!!!!!!!!";
}
