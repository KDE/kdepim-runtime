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

#include <kcal/incidence.h>
#include <kcal/event.h>
#include <kcal/todo.h>
#include <kcal/journal.h>
#include <kcal/kcalmimetypevisitor.h>
#include <kcal/calendarlocal.h>
#include <kcal/icalformat.h>

#include <KMime/Message>
#include <KMime/Content>

//#include <mailtransport/transport.h>
//#include <mailtransport/transporttype.h>
//#include <mailtransport/transportmanager.h>
//#include <mailtransport/smtpjob.h>
//#include <mailtransport/sendmailjob.h>
//#include <mailtransport/messagequeuejob.h>
//#include <mailtransport/sentbehaviourattribute.h>

//#include <Akonadi/Item>
//#include <Akonadi/ItemFetchJob>
//#include <Akonadi/ItemFetchScope>
//#include <Akonadi/ItemModifyJob>

//
//#include <Akonadi/CollectionCreateJob>
//#include <Akonadi/AgentManager>
//#include <Akonadi/AgentInstance>
//#include <akonadi/kmime/specialcollections.h>
//#include <akonadi/kmime/specialcollectionsrequestjob.h>

#include <kemailsettings.h>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>

#include <akonadi/kmime/specialcollections.h>
#include <akonadi/kmime/specialcollectionsrequestjob.h>
#include <akonadi/kcal/incidenceattribute.h>

#include <QFileInfo>
#include <KDebug>
#include <KSystemTimeZones>
#include <KJob>
#include <KLocale>

#include <kpimidentities/identitymanager.h>

AKONADI_AGENT_MAIN( InvitationsAgent )

using namespace Akonadi;

InvitationsAgentItem::InvitationsAgentItem(InvitationsAgent *a, const Akonadi::Item &originalItem)
  : QObject(a), a(a), originalItem(originalItem)
{
}

InvitationsAgentItem::~InvitationsAgentItem()
{
}

void InvitationsAgentItem::add(const Akonadi::Item &newItem)
{
  Akonadi::ItemCreateJob *j = new Akonadi::ItemCreateJob( newItem, a->invitations() );
  connect( j, SIGNAL( result( KJob* ) ), this, SLOT( createItemResult( KJob* ) ) );
  jobs << j;
  j->start();
}

void InvitationsAgentItem::createItemResult( KJob *job )
{
  Akonadi::ItemCreateJob *j = static_cast<Akonadi::ItemCreateJob*>( job );
  jobs.removeAll( j );
  if( j->error() ) {
    kWarning() << "Failed to create new Akonadi::Item in invitations collection." << j->errorText();
    return;
  }

  if( ! jobs.isEmpty() ) {
    return;
  }

  Akonadi::ItemFetchJob *fj = new Akonadi::ItemFetchJob( originalItem, this );
  connect( fj, SIGNAL( result( KJob* ) ), this, SLOT( fetchItemDone( KJob* ) ) );
  fj->start();
}

void InvitationsAgentItem::fetchItemDone( KJob *job )
{
  if( job->error() ) {
    kWarning() << "Failed to fetch Akonadi::Item in invitations collection." << job->errorText();
    return;
  }

  Akonadi::ItemFetchJob *fj = static_cast<Akonadi::ItemFetchJob*>( job );
  Q_ASSERT( fj->items().count() == 1 );
  Akonadi::Item modifiedItem = fj->items().first();
  Q_ASSERT( modifiedItem.isValid() );
  modifiedItem.setFlag( "invitation" );
  Akonadi::ItemModifyJob *mj = new Akonadi::ItemModifyJob( modifiedItem, this );
  connect( mj, SIGNAL( result( KJob* ) ), this, SLOT( modifyItemDone( KJob* ) ) );
  mj->start();  
}

void InvitationsAgentItem::modifyItemDone( KJob *job )
{
  if( job->error() ) {
    kWarning() << "Failed to modify Akonadi::Item in invitations collection." << job->errorText();
    return;
  }

  //Akonadi::ItemModifyJob *mj = static_cast<Akonadi::ItemModifyJob*>( job );
  //kDebug()<<"Job successful done.";
}

InvitationsAgent::InvitationsAgent( const QString &id )
  : Akonadi::AgentBase( id ), Akonadi::AgentBase::ObserverV2() //, m m_IdentityManager( 0 )
{
  kDebug();
  changeRecorder()->setChangeRecordingEnabled( false ); // behave like Akonadi::Monitor

  if( Akonadi::SpecialCollections::self()->hasDefaultCollection( Akonadi::SpecialCollections::Invitations ) ) {
    m_invitations = Akonadi::SpecialCollections::self()->defaultCollection( Akonadi::SpecialCollections::Invitations );
    Q_ASSERT( m_invitations.isValid() );
    init();
  } else {
    emit status( AgentBase::Running, "Loading..." );

    Akonadi::SpecialCollectionsRequestJob *j = new Akonadi::SpecialCollectionsRequestJob;
    connect( j, SIGNAL( result( KJob* ) ), this, SLOT( fetchInvitationCollectionResult( KJob* ) ) );
    j->requestDefaultCollection( Akonadi::SpecialCollections::Invitations );
    j->start();
  }
}

InvitationsAgent::~InvitationsAgent()
{
}

void InvitationsAgent::init()
{
  changeRecorder()->itemFetchScope().fetchFullPayload(); //FIXME don't fetch all!
  //changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::eventMimeType() );
  //changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::todoMimeType() );
  //changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::journalMimeType() );
  //changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::freeBusyMimeType() );
  //changeRecorder()->setMimeTypeMonitored( "text/calendar", true );
  changeRecorder()->setMimeTypeMonitored( "message/rfc822", true );
  //changeRecorder()->setCollectionMonitored( Collection::root(), true );
  
  emit status( AgentBase::Idle, "Ready to dispatch invitations" );
}

Akonadi::Collection& InvitationsAgent::invitations()
{
  return m_invitations;
}

#if 0
KPIMIdentities::IdentityManager* InvitationsAgent::identityManager()
{
  if( ! m_IdentityManager)
    m_IdentityManager = new KPIMIdentities::IdentityManager( true /* readonly */, this );
  return m_IdentityManager;
}
#endif

void InvitationsAgent::configure( WId windowId )
{
  kDebug() << windowId;
}

void InvitationsAgent::fetchInvitationCollectionResult( KJob *job )
{
  Akonadi::SpecialCollectionsRequestJob *j = static_cast<Akonadi::SpecialCollectionsRequestJob*>( job );
  if( j->error() ) {
    emit status( AgentBase::Broken, i18n("Failed to fetch the invitations collection: ",j->errorText()) );
    return;
  }
  m_invitations = j->collection();
  Q_ASSERT( m_invitations.isValid() );
  init();
}

#if 0
void InvitationsAgent::fetchCollectionResult( KJob *job )
{
  kDebug();
  /*
  Akonadi::CollectionFetchJob *j = static_cast<Akonadi::CollectionFetchJob*>( job );
  //foreach( const Akonadi::Collection &c, j->collections() )
  Akonadi::Collection collection;
  collection.setParent( Akonadi::Collection::root() );
  collection.setName( "Invitations" );
  collection.setContentMimeTypes( QStringList() << "text/calendar" );
  */
}
#endif

Akonadi::Item InvitationsAgent::handleContent( const QString &vcal, KCal::Calendar* calendar, const Akonadi::Item &item )
{
  KCal::ICalFormat format;
  KCal::ScheduleMessage *message = format.parseScheduleMessage( calendar, vcal );
  if( ! message ) {
    kWarning() << "Invalid invitation:" << vcal;
    return Akonadi::Item();
  }

  kDebug() << "id=" << item.id() << "remoteId=" << item.remoteId() << "vcal=" << vcal;

  KCal::Incidence* incidence = static_cast<KCal::Incidence*>( message->event() );
  Q_ASSERT( incidence );

  Akonadi::IncidenceAttribute *attr = new Akonadi::IncidenceAttribute;
  attr->setStatus( "new" ); //TODO
  //attr->setFrom( message->from()->asUnicodeString() );
  attr->setReference( item.id() );

  Akonadi::Item newItem;
  newItem.setMimeType( QString::fromLatin1("application/x-vnd.akonadi.calendar.%1").arg(QLatin1String(incidence->type().toLower())) );
  newItem.addAttribute( attr );
  newItem.setPayload<KCal::Incidence::Ptr>( KCal::Incidence::Ptr(incidence->clone()) );
  return newItem;
}

void InvitationsAgent::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  if( ! m_invitations.isValid() ) {
    return;
  }

  if( ! item.hasPayload<KMime::Message::Ptr>() ) {
    return;
  }

  KMime::Message::Ptr message = item.payload<KMime::Message::Ptr>();

  //TODO check if we are the sender and need to ignore the message...
  //const QString sender = message->sender()->asUnicodeString();
  //if( identityManager()->thatIsMe(sender) ) return;

  KCal::CalendarLocal calendar( KSystemTimeZones::local() ) ;
  if( message->contentType()->isMultipart() ) {
    InvitationsAgentItem *it = 0;
    foreach( KMime::Content *c, message->attachments() ) {
      KMime::Headers::ContentDisposition *ds = c->header< KMime::Headers::ContentDisposition >();
      if( !ds || QFileInfo(ds->filename()).suffix().toLower() != "ics" ) {
        continue;
      }
      Akonadi::Item newItem = handleContent( c->body(), &calendar, item );
      if( ! newItem.hasPayload() ) {
        continue;
      }
      if( ! it) {
        it = new InvitationsAgentItem( this, item );
      }
      it->add( newItem );
    }
  } else {

    //TODO check what is allowed/possible here.
    Akonadi::Item newItem = handleContent( message->body(), &calendar, item );
    if( ! newItem.hasPayload() ) {
      return;
    }
    InvitationsAgentItem *it = new InvitationsAgentItem( this, item );
    it->add( newItem );
  }

}

#if 0
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
#endif
