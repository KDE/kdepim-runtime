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
#include <kcal/calendarlocal.h>
#include <kcal/icalformat.h>

#include <KMime/Message>
#include <KMime/Content>

//#include <kemailsettings.h>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/CollectionCreateJob>
#include <Akonadi/AgentManager>
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentInstanceCreateJob>
#include <akonadi/resourcesynchronizationjob.h>
#include "incidenceattribute.h"
#include <akonadi/specialcollections.h>
#include <akonadi/specialcollectionsrequestjob.h>

#include <QDBusInterface>
#include <QDBusReply>
#include <QFileInfo>
#include <QTimer>
#include <KDebug>
#include <KSystemTimeZones>
#include <KJob>
#include <KLocale>
#include <KStandardDirs>
#include <KConfig>
#include <KConfigSkeleton>

#include <kpimidentities/identitymanager.h>

AKONADI_AGENT_MAIN( InvitationsAgent )

using namespace Akonadi;

class InvitationsCollectionRequestJob : public SpecialCollectionsRequestJob
{
  public:
    InvitationsCollectionRequestJob( SpecialCollections* collection, InvitationsAgent *agent ) : SpecialCollectionsRequestJob( collection, agent ) {
      setDefaultResourceType( QLatin1String( "akonadi_ical_resource" ) );

      QVariantMap options;
      options.insert( QLatin1String( "Path" ), KGlobal::dirs()->localxdgdatadir() + "akonadi_invitations" );
      options.insert( QLatin1String( "Name" ), i18n("Invitations") );
      setDefaultResourceOptions( options );

      QMap<QByteArray, QString> displayNameMap;
      displayNameMap.insert( "invitations", i18n("Invitations") );
      setTypes( displayNameMap.keys() );
      setNameForTypeMap( displayNameMap );

      QMap<QByteArray, QString> iconNameMap;
      iconNameMap.insert( "invitations", QLatin1String( "folder" ) );
      setIconForTypeMap( iconNameMap );
    }
    virtual ~InvitationsCollectionRequestJob() {}
};

class  InvitationsCollection : public SpecialCollections
{
  public:

    class Settings : public KCoreConfigSkeleton
    {
      public:
        Settings() : KCoreConfigSkeleton() {
          setCurrentGroup("Invitations");
          addItemString("DefaultResourceId", m_id, QString());
        }
        virtual ~Settings() {}
      private:
        QString m_id;
    };

    InvitationsCollection( InvitationsAgent *agent ) : Akonadi::SpecialCollections( new Settings ), m_agent( agent ), sInvitationsType( "invitations" ) {}
    virtual ~InvitationsCollection() {}
    /*
    bool registerCollection( const Collection &collection ) {
      return SpecialCollections::registerCollection( sInvitationsType, collection );
    }
    */
    void clear() {
      m_collection = Collection();
    }
    bool hasDefaultCollection() const {
      return SpecialCollections::hasDefaultCollection( sInvitationsType );
    }
    Collection defaultCollection() const {
      if( ! m_collection.isValid() )
        m_collection = SpecialCollections::defaultCollection( sInvitationsType );
      return m_collection;
    }
    void registerDefaultCollection() {
//      Q_ASSERT( m_collection.isValid() );
      defaultCollection();
      registerCollection( sInvitationsType, m_collection );
    }
    SpecialCollectionsRequestJob* reguestJob() const {
      InvitationsCollectionRequestJob *j = new InvitationsCollectionRequestJob( const_cast<InvitationsCollection*>(this), m_agent );
      j->requestDefaultCollection( sInvitationsType );
      return j;
    }
  private:
    InvitationsAgent *m_agent;
    const QByteArray sInvitationsType;
    mutable Collection m_collection;
};

InvitationsAgentItem::InvitationsAgentItem(InvitationsAgent *a, const Item &originalItem)
  : QObject(a), a(a), originalItem(originalItem)
{
}

InvitationsAgentItem::~InvitationsAgentItem()
{
}

void InvitationsAgentItem::add(const Item &newItem)
{
  kDebug();
  Collection c = a->collection();
  Q_ASSERT( c.isValid() );

  ItemCreateJob *j = new ItemCreateJob( newItem, c, this );
  jobs << j;
  connect( j, SIGNAL( result( KJob* ) ), this, SLOT( createItemResult( KJob* ) ) );
  j->start();
}

void InvitationsAgentItem::createItemResult( KJob *job )
{
  ItemCreateJob *j = static_cast<ItemCreateJob*>( job );
  jobs.removeAll( j );
  if( j->error() ) {
    kWarning() << "Failed to create new Item in invitations collection." << j->errorText();
    return;
  }

  if( ! jobs.isEmpty() ) {
    return;
  }

  ItemFetchJob *fj = new ItemFetchJob( originalItem, this );
  connect( fj, SIGNAL( result( KJob* ) ), this, SLOT( fetchItemDone( KJob* ) ) );
  fj->start();
}

void InvitationsAgentItem::fetchItemDone( KJob *job )
{
  if( job->error() ) {
    kWarning() << "Failed to fetch Item in invitations collection." << job->errorText();
    return;
  }

  ItemFetchJob *fj = static_cast<ItemFetchJob*>( job );
  Q_ASSERT( fj->items().count() == 1 );
  Item modifiedItem = fj->items().first();
  Q_ASSERT( modifiedItem.isValid() );
  modifiedItem.setFlag( "invitation" );
  ItemModifyJob *mj = new ItemModifyJob( modifiedItem, this );
  connect( mj, SIGNAL( result( KJob* ) ), this, SLOT( modifyItemDone( KJob* ) ) );
  mj->start();  
}

void InvitationsAgentItem::modifyItemDone( KJob *job )
{
  if( job->error() ) {
    kWarning() << "Failed to modify Item in invitations collection." << job->errorText();
    return;
  }

  //ItemModifyJob *mj = static_cast<ItemModifyJob*>( job );
  //kDebug()<<"Job successful done.";
}

InvitationsAgent::InvitationsAgent( const QString &id )
  : AgentBase( id ), AgentBase::ObserverV2()
  , m_InvitationsCollection( new InvitationsCollection(this) )
{
  kDebug();
  KGlobal::locale()->insertCatalog( "akonadi_invitations_agent" );

  changeRecorder()->setChangeRecordingEnabled( false ); // behave like Monitor
  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->setMimeTypeMonitored( "message/rfc822", true );
  //changeRecorder()->setCollectionMonitored( Collection::root(), true );

  connect( this, SIGNAL(reloadConfiguration()), this, SLOT(initStart()) );
  QTimer::singleShot( 0, this, SLOT(initStart()) );
}

InvitationsAgent::~InvitationsAgent()
{
  delete m_InvitationsCollection;
}

void InvitationsAgent::initStart()
{
  kDebug();

  m_InvitationsCollection->clear();
  if( m_InvitationsCollection->hasDefaultCollection() ) {
    initDone();
  } else {
    SpecialCollectionsRequestJob *j = m_InvitationsCollection->reguestJob();
    connect( j, SIGNAL( result(KJob*) ), this, SLOT( initDone(KJob*) ) );
    j->start();
  }

  /*
  KConfig config( "akonadi_invitations_agent" );
  KConfigGroup group = config.group( "General" );
  m_resourceId = group.readEntry( "DefaultCalendarAgent", "default_ical_resource" );
  newAgentCreated = false;
  m_invitations = Akonadi::Collection();
  AgentInstance resource = AgentManager::self()->instance( m_resourceId );
  if( resource.isValid() ) {
    emit status( AgentBase::Running, i18n("Reading...") );
    QMetaObject::invokeMethod( this, "createAgentResult", Qt::QueuedConnection );
  } else {
    emit status( AgentBase::Running, i18n("Creating...") );
    AgentType type = AgentManager::self()->type( QLatin1String("akonadi_ical_resource") );
    AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type, this );
    connect( job, SIGNAL( result( KJob * ) ), this, SLOT( createAgentResult( KJob * ) ) );
    job->start();
  }
  */
}

void InvitationsAgent::initDone( KJob *job )
{
  if( job ) {
    if( job->error() ) {
      kWarning() << "Failed to request default collection:" << job->errorText();
      return;
    }
    m_InvitationsCollection->registerDefaultCollection();
  }

  Q_ASSERT( m_InvitationsCollection->defaultCollection().isValid() );
  emit status( AgentBase::Idle, i18n("Ready to dispatch invitations") );
}

Collection InvitationsAgent::collection()
{
  return m_InvitationsCollection->defaultCollection();
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
  /*
  QWidget *parent = QWidget::find( windowId );
  KDialog *dialog = new KDialog( parent );
  QVBoxLayout *layout = new QVBoxLayout( dialog->mainWidget() );
  //layout->addWidget(  );
  dialog->mainWidget()->setLayout( layout );
  */
  initStart(); //reload
}

#if 0
void InvitationsAgent::createAgentResult( KJob *job )
{
  kDebug();
  AgentInstance agent;
  if( job ) {
    if( job->error() ) {
      kWarning() << job->errorString();
      emit status( AgentBase::Broken, i18n( "Failed to create resource: %1", job->errorString() ) );
      return;
    }

    AgentInstanceCreateJob *j = static_cast<AgentInstanceCreateJob*>( job );
    agent = j->instance();
    agent.setName( i18n("Invitations") );
    m_resourceId = agent.identifier();

    QDBusInterface conf( QString::fromLatin1( "org.freedesktop.Akonadi.Resource." ) + m_resourceId,
                         QString::fromLatin1( "/Settings" ),
                         QString::fromLatin1( "org.kde.Akonadi.ICal.Settings" ) );
    QDBusReply<void> reply = conf.call( QString::fromLatin1( "setPath" ),
                                        KGlobal::dirs()->localxdgdatadir() + "akonadi_ical_resource" );

    if( !reply.isValid() ) {
      kWarning() << "dbus call failed, m_resourceId=" << m_resourceId;
      emit status( AgentBase::Broken, i18n( "Failed to set the directory for invitations via D-Bus" ) );
      AgentManager::self()->removeInstance( agent );
      return;
    }

    KConfig config( "akonadi_invitations_agent" );
    KConfigGroup group = config.group( "General" );
    group.writeEntry( "DefaultCalendarAgent", m_resourceId );

    newAgentCreated = true;
    agent.reconfigure();
  } else {
    agent = AgentManager::self()->instance( m_resourceId );
    Q_ASSERT( agent.isValid() );
  }

  ResourceSynchronizationJob *j = new ResourceSynchronizationJob( agent, this );
  connect( j, SIGNAL(result(KJob*)), this, SLOT(resourceSyncResult(KJob*)) );
  j->start();
}

void InvitationsAgent::resourceSyncResult( KJob *job )
{
  kDebug();
  if( job->error() ) {
    kWarning() << job->errorString();
    emit status( AgentBase::Broken, i18n( "Failed to synchronize collection: %1", job->errorString() ) );
    if( newAgentCreated )
      AgentManager::self()->removeInstance( AgentManager::self()->instance( m_resourceId ) );
    return;
  }
  CollectionFetchJob *fjob = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
  fjob->fetchScope().setContentMimeTypes( QStringList() << "text/calendar" );
  fjob->fetchScope().setResource( m_resourceId );
  connect( fjob, SIGNAL(result(KJob*)), this, SLOT(collectionFetchResult(KJob*)) );
  fjob->start();
}

void InvitationsAgent::collectionFetchResult( KJob *job )
{
  kDebug();

  if( job->error() ) {
    kWarning() << job->errorString();
    emit status( AgentBase::Broken, i18n( "Failed to fetch collection: %1", job->errorString() ) );
    if( newAgentCreated )
      AgentManager::self()->removeInstance( AgentManager::self()->instance( m_resourceId ) );
    return;
  }

  CollectionFetchJob *fj = static_cast<CollectionFetchJob *>( job );

  if( newAgentCreated ) {
    // if the agent was just created then there is exactly one collection already
    // and we just use that one.
    Q_ASSERT( fj->collections().count() == 1 );
    m_invitations = fj->collections().first();
    initDone();
    return;
  }

  KConfig config( "akonadi_invitations_agent" );
  KConfigGroup group = config.group( "General" );
  const QString collectionId = group.readEntry( "DefaultCalendarCollection", QString() );
  if( ! collectionId.isEmpty() ) {
    // look if the collection is still there. It may the case that there exists such
    // a collection with the defined collectionId but that this is not a valid one
    // and therefore not in the resultset.
    const int id = collectionId.toInt();
    foreach( const Collection &c, fj->collections() ) {
      if( c.id() == id ) {
        m_invitations = c;
        initDone();
        return;
      }
    }
  }

  // we need to create a new collection and use that one...
  Collection c;
  c.setName( "invitations" );
  c.setParent( Collection::root() );
  Q_ASSERT( ! m_resourceId.isNull() );
  c.setResource( m_resourceId );
  c.setContentMimeTypes( QStringList()
            << "text/calendar"
            << "application/x-vnd.akonadi.calendar.event"
            << "application/x-vnd.akonadi.calendar.todo"
            << "application/x-vnd.akonadi.calendar.journal"
            << "application/x-vnd.akonadi.calendar.freebusy" );
  CollectionCreateJob *cj = new CollectionCreateJob( c, this );
  connect( cj, SIGNAL(result(KJob*)), this, SLOT(collectionCreateResult(KJob*)) );
  cj->start();
}

void InvitationsAgent::collectionCreateResult( KJob *job )
{
  kDebug();
  if( job->error() ) {
    kWarning() << job->errorString();
    emit status( AgentBase::Broken, i18n( "Failed to create collection: %1", job->errorString() ) );
    if( newAgentCreated )
      AgentManager::self()->removeInstance( AgentManager::self()->instance( m_resourceId ) );
    return;
  }
  CollectionCreateJob *j = static_cast<CollectionCreateJob*>( job );
  m_invitations = j->collection();
  initDone();
}
#endif

Item InvitationsAgent::handleContent( const QString &vcal, KCal::Calendar* calendar, const Item &item )
{
  KCal::ICalFormat format;
  KCal::ScheduleMessage *message = format.parseScheduleMessage( calendar, vcal );
  if( ! message ) {
    kWarning() << "Invalid invitation:" << vcal;
    return Item();
  }

  kDebug() << "id=" << item.id() << "remoteId=" << item.remoteId() << "vcal=" << vcal;

  KCal::Incidence* incidence = static_cast<KCal::Incidence*>( message->event() );
  Q_ASSERT( incidence );

  IncidenceAttribute *attr = new IncidenceAttribute;
  attr->setStatus( "new" ); //TODO
  //attr->setFrom( message->from()->asUnicodeString() );
  attr->setReference( item.id() );

  Item newItem;
  newItem.setMimeType( QString::fromLatin1("application/x-vnd.akonadi.calendar.%1").arg(QLatin1String(incidence->type().toLower())) );
  newItem.addAttribute( attr );
  newItem.setPayload<KCal::Incidence::Ptr>( KCal::Incidence::Ptr(incidence->clone()) );
  return newItem;
}

void InvitationsAgent::itemAdded( const Item &item, const Collection &collection )
{
  kDebug() << item.id() << collection;
  Q_UNUSED( collection );

  if( ! m_InvitationsCollection->defaultCollection().isValid() ) {
    kDebug() << "No default collection found";
    return;
  }

  if( ! item.hasPayload<KMime::Message::Ptr>() ) {
    kDebug() << "Item has no payload";
    return;
  }

  KMime::Message::Ptr message = item.payload<KMime::Message::Ptr>();

  //TODO check if we are the sender and need to ignore the message...
  //const QString sender = message->sender()->asUnicodeString();
  //if( identityManager()->thatIsMe(sender) ) return;

  KCal::CalendarLocal calendar( KSystemTimeZones::local() ) ;
  if( message->contentType()->isMultipart() ) {
    kDebug() << "message is multipart:" << message->attachments().size();
    InvitationsAgentItem *it = 0;
    foreach( KMime::Content *c, message->attachments() ) {
      KMime::Headers::ContentDisposition *ds = c->header< KMime::Headers::ContentDisposition >();
      kDebug() << "looking at " << ds << (ds ? QFileInfo(ds->filename()).suffix().toLower() : QString());
      if( !ds || QFileInfo(ds->filename()).suffix().toLower() != "ics" ) {
        kDebug() << "no CD header or not an ics file";
        continue;
      }
      Item newItem = handleContent( c->body(), &calendar, item );
      if( ! newItem.hasPayload() ) {
        kDebug() << "new item has no payload";
        continue;
      }
      if( ! it) {
        it = new InvitationsAgentItem( this, item );
      }
      it->add( newItem );
    }
  } else {
    kDebug() << "message is not multipart";
    //TODO check what is allowed/possible here.
    Item newItem = handleContent( message->body(), &calendar, item );
    if( ! newItem.hasPayload() ) {
      kDebug() << "new item has no payload";
      return;
    }
    InvitationsAgentItem *it = new InvitationsAgentItem( this, item );
    it->add( newItem );
  }

}
