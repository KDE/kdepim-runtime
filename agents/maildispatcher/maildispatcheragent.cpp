/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "maildispatcheragent.h"

//#include "configdialog.h"
#include "maildispatcheragentadaptor.h"
#include "outboxqueue.h"
#include "sendjob.h"
#include "sentactionhandler.h"
#include "settings.h"
#include "settingsadaptor.h"

#include <kpimutils/networkaccesshelper.h>
#include <akonadi/agentfactory.h>
#include <akonadi/dbusconnectionpool.h>
#include <akonadi/itemfetchscope.h>
#include <mailtransport/sentactionattribute.h>

#include <knotifyconfigwidget.h>
#include <KDebug>
#include <KIcon>
#include <KIconLoader>
#include <KLocale>
#include <KLocalizedString>
#include <KMime/Message>
#include <KNotification>
#include <KWindowSystem>

#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>

#ifdef KDEPIM_STATIC_LIBS
extern bool ___MailTransport____INIT();
#endif

#ifdef MAIL_SERIALIZER_PLUGIN_STATIC
#include <QtPlugin>

Q_IMPORT_PLUGIN(akonadi_serializer_mail)
#endif

using namespace Akonadi;

class MailDispatcherAgent::Private
{
  public:
    Private( MailDispatcherAgent *parent )
      : q( parent ),
        currentJob( 0 ),
        aborting( false ),
        sendingInProgress( false ),
        sentAnything( false ),
        errorOccurred( false ),
        networkAccessHelper( new KPIMUtils::NetworkAccessHelper( parent ) )
    {
    }

    ~Private()
    {
    }

    MailDispatcherAgent * const q;

    OutboxQueue *queue;
    SendJob *currentJob;
    Item currentItem;
    bool aborting;
    bool sendingInProgress;
    bool sentAnything;
    bool errorOccurred;
    qulonglong sentItemsSize;
    SentActionHandler *sentActionHandler;
    KPIMUtils::NetworkAccessHelper *networkAccessHelper;

    // slots:
    void abort();
    void dispatch();
    void itemFetched( const Item &item );
    void queueError( const QString &message );
    void sendPercent( KJob *job, unsigned long percent );
    void sendResult( KJob *job );
    void emitStatusReady();
};


void MailDispatcherAgent::Private::abort()
{
  if ( !q->isOnline() ) {
    kDebug() << "Offline. Ignoring call.";
    return;
  }

  if ( aborting ) {
    kDebug() << "Already aborting.";
    return;
  }

  if ( !sendingInProgress && queue->isEmpty() ) {
    kDebug() << "MDA is idle.";
    Q_ASSERT( q->status() == AgentBase::Idle );
  } else {
    kDebug() << "Aborting...";
    aborting = true;
    if ( currentJob ) {
      currentJob->abort();
    }
    // Further SendJobs will mark remaining items in the queue as 'aborted'.
  }
}

void MailDispatcherAgent::Private::dispatch()
{
  Q_ASSERT( queue );
  networkAccessHelper->establishConnection();

  if ( !q->isOnline() || sendingInProgress ) {
    kDebug() << "Offline or busy. See you later.";
    return;
  }

  if ( !queue->isEmpty() ) {
    if ( !sentAnything ) {
      sentAnything = true;
      sentItemsSize = 0;
      emit q->percent( 0 );
    }
    emit q->status( AgentBase::Running,
        i18np( "Sending messages (1 item in queue)...",
               "Sending messages (%1 items in queue)...", queue->count() ) );
    kDebug() << "Attempting to dispatch the next message.";
    sendingInProgress = true;
    queue->fetchOne(); // will trigger itemFetched
  } else {
    kDebug() << "Empty queue.";
    if ( aborting ) {
      // Finished marking messages as 'aborted'.
      aborting = false;
      sentAnything = false;
      emit q->status( AgentBase::Idle, i18n( "Sending canceled." ) );
      QTimer::singleShot( 3000, q, SLOT( emitStatusReady() ) );
    } else {
      if ( sentAnything ) {
        // Finished sending messages in queue.
        sentAnything = false;
        emit q->percent( 100 );
        emit q->status( AgentBase::Idle, i18n( "Finished sending messages." ) );

        if ( !errorOccurred ) {
          const QPixmap pixmap = KIcon( "mail-folder-outbox" ).pixmap( KIconLoader::SizeSmall, KIconLoader::SizeSmall );
          KNotification *notify = new KNotification( "emailsent" );
          notify->setComponentData( q->componentData() );
          notify->setPixmap( pixmap );
          notify->setText( i18nc("Notification when the email was sent", "E-mail successfully sent" ) );
          notify->sendEvent();
        }
      } else {
        // Empty queue.
        emit q->status( AgentBase::Idle, i18n( "No items in queue." ) );
      }
      QTimer::singleShot( 3000, q, SLOT( emitStatusReady() ) );
    }

    errorOccurred = false;
  }
}


MailDispatcherAgent::MailDispatcherAgent( const QString &id )
  : AgentBase( id ),
    d( new Private( this ) )
{
  kDebug() << "maildispatcheragent: At your service, sir!";

#ifdef _WIN32_WCE
  QThread::currentThread()->setPriority(QThread::NormalPriority);
#endif

#ifdef KDEPIM_STATIC_LIBS
    ___MailTransport____INIT();
#endif

  KGlobal::locale()->insertCatalog( "libakonadi-kmime" ); // for special collection translation

  new SettingsAdaptor( Settings::self() );
  new MailDispatcherAgentAdaptor( this );

  DBusConnectionPool::threadConnection().registerObject( QLatin1String( "/Settings" ),
                                                         Settings::self(), QDBusConnection::ExportAdaptors );

  DBusConnectionPool::threadConnection().registerObject( QLatin1String( "/MailDispatcherAgent" ),
                                                         this, QDBusConnection::ExportAdaptors );
  DBusConnectionPool::threadConnection().registerService( QLatin1String( "org.freedesktop.Akonadi.MailDispatcherAgent" ) );

  d->queue = new OutboxQueue( this );
  connect( d->queue, SIGNAL( newItems() ),
           this, SLOT( dispatch() ) );
  connect( d->queue, SIGNAL( itemReady( const Akonadi::Item& ) ),
           this, SLOT( itemFetched( const Akonadi::Item& ) ) );
  connect( d->queue, SIGNAL( error( const QString& ) ),
           this, SLOT( queueError( const QString& ) ) );
  connect( this, SIGNAL( itemProcessed( Akonadi::Item, bool ) ),
           d->queue, SLOT( itemProcessed( Akonadi::Item, bool ) ) );
  connect( this, SIGNAL( abortRequested() ),
           this, SLOT( abort() ) );

  d->sentActionHandler = new SentActionHandler( this );

  setNeedsNetwork( true );
}

MailDispatcherAgent::~MailDispatcherAgent()
{
  delete d;
}

void MailDispatcherAgent::configure( WId windowId )
{
  Q_UNUSED( windowId );
  KNotifyConfigWidget::configure( 0 );
}

void MailDispatcherAgent::doSetOnline( bool online )
{
  Q_ASSERT( d->queue );
  if ( online ) {
    kDebug() << "Online. Dispatching messages.";
    emit status( AgentBase::Idle, i18n( "Online, sending messages in queue." ) );
    QTimer::singleShot( 0, this, SLOT( dispatch() ) );
  } else {
    kDebug() << "Offline.";
    emit status( AgentBase::Idle, i18n( "Offline, message sending suspended." ) );

    // TODO: This way, the OutboxQueue will continue to react to changes in
    // the outbox, but the MDA will just not send anything.  Is this what we
    // want?
  }

  AgentBase::doSetOnline( online );
}

void MailDispatcherAgent::Private::itemFetched( const Item &item )
{
  kDebug() << "Fetched item" << item.id() << "; creating SendJob.";
  Q_ASSERT( sendingInProgress );
  Q_ASSERT( !currentItem.isValid() );
  currentItem = item;
  Q_ASSERT( currentJob == 0 );
  emit q->itemDispatchStarted();

  currentJob = new SendJob( item, q );
  if ( aborting ) {
    currentJob->setMarkAborted();
  }

  q->status( AgentBase::Running, i18nc( "Message with given subject is being sent.", "Sending: %1",
                                        item.payload<KMime::Message::Ptr>()->subject()->asUnicodeString() ) );

  connect( currentJob, SIGNAL( result( KJob* ) ),
           q, SLOT( sendResult( KJob* ) ) );
  connect( currentJob, SIGNAL( percent( KJob*, unsigned long ) ),
           q, SLOT( sendPercent( KJob*, unsigned long ) ) );

  currentJob->start();
}

void MailDispatcherAgent::Private::queueError( const QString &message )
{
  emit q->error( message );
  errorOccurred = true;
  // FIXME figure out why this does not set the status to Broken, etc.
}

void MailDispatcherAgent::Private::sendPercent( KJob *job, unsigned long )
{
  Q_ASSERT( sendingInProgress );
  Q_ASSERT( job == currentJob );
  // The progress here is actually the TransportJob, not the entire SendJob,
  // because the post-job doesn't report progress.  This should be fine,
  // since the TransportJob is the lengthiest operation.

  // Give the transport 80% of the weight, and move-to-sendmail 20%.
  const double transportWeight = 0.8;

  const int percent = 100 * ( sentItemsSize + job->processedAmount( KJob::Bytes ) * transportWeight )
                          / ( sentItemsSize + currentItem.size() + queue->totalSize() );

  kDebug() << "sentItemsSize" << sentItemsSize
           << "this job processed" << job->processedAmount( KJob::Bytes )
           << "queue totalSize" << queue->totalSize()
           << "total total size (sent+current+queue)" << ( sentItemsSize + currentItem.size() + queue->totalSize() )
           << "new percentage" << percent << "old percentage" << q->progress();

  if ( percent != q->progress() ) {
    // The progress can decrease too, if messages got added to the queue.
    emit q->percent( percent );
  }

  // It is possible that the number of queued messages has changed.
  emit q->status( AgentBase::Running,
      i18np( "Sending messages (1 item in queue)...",
             "Sending messages (%1 items in queue)...", 1 + queue->count() ) );
}

void MailDispatcherAgent::Private::sendResult( KJob *job )
{
  Q_ASSERT( sendingInProgress );
  Q_ASSERT( job == currentJob );
  currentJob->disconnect( q );
  currentJob = 0;

  Q_ASSERT( currentItem.isValid() );
  sentItemsSize += currentItem.size();
  emit q->itemProcessed( currentItem, !job->error() );

  const Akonadi::Item sentItem = currentItem;
  currentItem = Item();

  if ( job->error() ) {
    // The SendJob gave the item an ErrorAttribute, so we don't have to
    // do anything.
    kDebug() << "Sending failed. error:" << job->errorString();

    const QPixmap pixmap = KIcon( "dialog-error" ).pixmap( KIconLoader::SizeSmall, KIconLoader::SizeSmall );
    KNotification *notify = new KNotification( "sendingfailed" );
    notify->setComponentData( q->componentData() );
    notify->setPixmap( pixmap );
    notify->setText( i18nc("Notification when email sending failed", "E-mail sending failed: %1", job->errorString() ) );
    notify->sendEvent();

    errorOccurred = true;
  } else {
    kDebug() << "Sending succeeded.";

    // handle possible sent actions
    const MailTransport::SentActionAttribute *attribute = sentItem.attribute<MailTransport::SentActionAttribute>();
    if ( attribute ) {
      foreach ( const MailTransport::SentActionAttribute::Action &action, attribute->actions() )
        sentActionHandler->runAction( action );
    }
  }

  // dispatch next message
  sendingInProgress = false;
  QTimer::singleShot( 0, q, SLOT( dispatch() ) );
}

void MailDispatcherAgent::Private::emitStatusReady()
{
  if ( q->status() == AgentBase::Idle ) {
    // If still idle after aborting, clear 'aborted' status.
    emit q->status( AgentBase::Idle, i18n( "Ready to dispatch messages." ) );
  }
}

#ifdef KDEPIM_PLUGIN_AGENT
AKONADI_AGENT_FACTORY( MailDispatcherAgent, akonadi_maildispatcher_agent )
#else
AKONADI_AGENT_MAIN( MailDispatcherAgent)
#endif

#include "maildispatcheragent.moc"
