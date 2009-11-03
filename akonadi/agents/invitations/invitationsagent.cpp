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
//#include <kmime/kmime_content.h>
//#include <kmime/kmime_message.h>

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
//#include <Akonadi/ItemCreateJob>
//#include <Akonadi/ItemModifyJob>
//#include <Akonadi/CollectionFetchJob>
//#include <Akonadi/CollectionFetchScope>
//#include <Akonadi/CollectionCreateJob>
//#include <Akonadi/AgentManager>
//#include <Akonadi/AgentInstance>
//#include <akonadi/kmime/specialcollections.h>
//#include <akonadi/kmime/specialcollectionsrequestjob.h>

#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>

#include <QFileInfo>
#include <KDebug>
#include <KSystemTimeZones>
#include <KJob>

AKONADI_AGENT_MAIN( InvitationsAgent )

using namespace Akonadi;

/*
class KORGANIZER_INTERFACES_EXPORT CalendarAdaptor : public KCal::Calendar
{
  public:
    explicit CalendarAdaptor() : KCal::Calendar( KOPrefs::instance()->timeSpec() ) {}
    virtual ~CalendarAdaptor() {}
    virtual bool save() { Q_ASSERT(false); return true; } //unused
    virtual bool reload() { Q_ASSERT(false); return true; } //unused
    virtual void close() { Q_ASSERT(false); } //unused
    virtual bool addEvent( Event *event ) { Q_ASSERT(false); return true; }
    virtual bool deleteEvent( Event *event ) { Q_ASSERT(false); return true; }
    virtual void deleteAllEvents() { Q_ASSERT(false); } //unused
    virtual Event::List rawEvents(KCal::EventSortField sortField = KCal::EventSortUnsorted, KCal::SortDirection sortDirection = KCal::SortDirectionAscending ) { Q_ASSERT(false); return Event::List; }
    virtual Event::List rawEventsForDate( const KDateTime &dt ) { Q_ASSERT(false); return Event::List; }
    virtual Event::List rawEvents( const QDate &start, const QDate &end, const KDateTime::Spec &timeSpec = KDateTime::Spec(), bool inclusive = false )  { Q_ASSERT(false); return Event::List; }
    virtual Event::List rawEventsForDate( const QDate &date, const KDateTime::Spec &timeSpec = KDateTime::Spec(), KCal::EventSortField sortField = KCal::EventSortUnsorted, KCal::SortDirection sortDirection = KCal::SortDirectionAscending ) { Q_ASSERT(false); return Event::List; }
    virtual Event *event( const QString &uid ) { Q_ASSERT(false); return 0; }
    virtual bool addTodo( Todo *todo ) { Q_ASSERT(false); return true; }
    virtual bool deleteTodo( Todo *todo ) { Q_ASSERT(false); return true; }
    virtual void deleteAllTodos() { Q_ASSERT(false); } //unused
    virtual Todo::List rawTodos( KCal::TodoSortField sortField = KCal::TodoSortUnsorted, KCal::SortDirection sortDirection = KCal::SortDirectionAscending ) { Q_ASSERT(false); return Todo::List; }
    virtual Todo::List rawTodosForDate( const QDate &date ) { Q_ASSERT(false); return Todo::List; }
    virtual Todo *todo( const QString &uid ) { Q_ASSERT(false); return 0; }
    virtual bool addJournal( Journal *journal ) { Q_ASSERT(false); return true; }
    virtual bool deleteJournal( Journal *journal ) { Q_ASSERT(false); return true; }
    virtual void deleteAllJournals() { Q_ASSERT(false); }
    virtual Journal::List rawJournals( KCal::JournalSortField sortField = KCal::JournalSortUnsorted, KCal::SortDirection sortDirection = KCal::SortDirectionAscending ) { Q_ASSERT(false); return Journal::List; }
    virtual Journal::List rawJournalsForDate( const QDate &dt ) { Q_ASSERT(false); return Journal::List; }
    virtual Journal *journal( const QString &uid ) { Q_ASSERT(false); return 0; }
    virtual Alarm::List alarms( const KDateTime &from, const KDateTime &to ) { Q_ASSERT(false); return Alarm::List; }
};
*/

InvitationsAgent::InvitationsAgent( const QString &id )
  : Akonadi::AgentBase( id )
  , Akonadi::AgentBase::ObserverV2()
{
  kDebug() << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";

  changeRecorder()->setChangeRecordingEnabled( false ); // behave like Akonadi::Monitor
  changeRecorder()->itemFetchScope().fetchFullPayload(); //FIXME don't fetch all!
  //changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::eventMimeType() );
  //changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::todoMimeType() );
  //changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::journalMimeType() );
  //changeRecorder()->setMimeTypeMonitored( KCalMimeTypeVisitor::freeBusyMimeType() );
  //changeRecorder()->setMimeTypeMonitored( "text/calendar", true );
  changeRecorder()->setMimeTypeMonitored( "message/rfc822", true );
  //changeRecorder()->setCollectionMonitored( Collection::root(), true );

  //setNeedsNetwork( true );
  //setOnline( true );
  emit status( AgentBase::Idle, "Ready to move invitations into calendars" );
}

InvitationsAgent::~InvitationsAgent()
{
  kDebug() << "!!!!!!!!!!!!!!!!!!!";
}

class MyCollectionFetchJob : public Akonadi::CollectionFetchJob
{
  public:
    explicit MyCollectionFetchJob( KCal::ScheduleMessage *message )
      : Akonadi::CollectionFetchJob( Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive )
      , m_message( message )
    {
    }
    ~MyCollectionFetchJob()
    {
    }
    KCal::ScheduleMessage *m_message;
};

class MyItemFetchJob : public Akonadi::ItemFetchJob
{
  public:
    MyItemFetchJob( const Akonadi::Collection &c, KCal::ScheduleMessage *message )
      : Akonadi::ItemFetchJob( c )
      , m_message( message )
    {
    }
    ~MyItemFetchJob()
    {
    }
    KCal::ScheduleMessage *m_message;
};

bool InvitationsAgent::handleInvitation( const QString &vcal )
{
  KCal::CalendarLocal calendar( KSystemTimeZones::local() ) ;
  KCal::ICalFormat format;
  KCal::ScheduleMessage *message = format.parseScheduleMessage( &calendar, vcal );
  if( ! message ) {
    kWarning() << "Invalid invitation:" << vcal;
    return false;
  }

  Q_ASSERT( message->event() );

  Akonadi::CollectionFetchJob *cfj = new MyCollectionFetchJob( message );
  Akonadi::CollectionFetchScope cfs;
  cfs.setIncludeStatistics( false );
  cfs.setContentMimeTypes( QStringList() << "text/calendar" );
  cfj->setFetchScope(cfs);
  connect( cfj, SIGNAL( result( KJob* ) ), this, SLOT( fetchCollectionResult( KJob* ) ) );
  cfj->start();

  /*
  const QString query = "SELECT ?id { ?remoteId \""+ incidence->uid() +"\" }";
  Akonadi::ItemSearchJob* job = new Akonadi::ItemSearchJob( query );
  job->fetchScope().fetchFullPayload();
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( searchResult( KJob* ) ) );
  */

  return true;
}

void InvitationsAgent::fetchCollectionResult( KJob *job )
{
  MyCollectionFetchJob *cfj = static_cast<MyCollectionFetchJob*>( job );
  KCal::Incidence *incidence = static_cast<KCal::Incidence*>( cfj->m_message->event() );
  Q_ASSERT( incidence );

  foreach( const Akonadi::Collection &c, cfj->collections() ) {
    if( ! c.contentMimeTypes().contains("text/calendar") )
      continue;

    Akonadi::ItemFetchJob *ifj = new MyItemFetchJob( c, cfj->m_message );
    ItemFetchScope ifs;
    ifs.fetchAllAttributes( true );
    ifs.fetchFullPayload( true );
    ifj->setFetchScope( ifs );
    connect( ifj, SIGNAL( result( KJob* ) ), this, SLOT( fetchItemResult( KJob* ) ) );
    ifj->start();
  }
}

void InvitationsAgent::fetchItemResult( KJob *job )
{
  MyItemFetchJob *ifj = static_cast<MyItemFetchJob*>( job );
  foreach( const Akonadi::Item &itm, ifj->items() ) {
    if( ! itm.hasPayload<KCal::Incidence::Ptr>() )
      continue;

    KCal::Incidence::Ptr inc = itm.payload<KCal::Incidence::Ptr>();
    if( itm.remoteId() != inc->uid() )
      continue;

    // time to deliver the new KCal::ScheduleMessage...
    kDebug() << "status=" << ifj->m_message->status() << "item=" << inc->uid() << inc->summary();

    //TODO
    
  }
}

void InvitationsAgent::configure( WId windowId )
{
  kDebug() << windowId;
}

void InvitationsAgent::doSetOnline( bool online )
{
  kDebug() << online;
}

void InvitationsAgent::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  if( ! item.hasPayload<KMime::Message::Ptr>() )
    return;

  // check if there is an invitation attachment
  KMime::Message::Ptr message = item.payload<KMime::Message::Ptr>();
  if( message->contentType()->isMultipart() ) {
    foreach( KMime::Content *c, message->attachments() ) {
      KMime::Headers::ContentDisposition *ds = c->header< KMime::Headers::ContentDisposition >();
      Q_ASSERT(ds);
      if( ds && c->hasContent() && QFileInfo(ds->filename()).suffix().toLower() == "ics" ) {
        kDebug() << "id=" << item.id() << "remoteId=" << item.remoteId() << "attachment=" << ds->filename() << "bidy=" << c->body();
        handleInvitation( c->body() );
      }
    }
  }
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
