/*
  Copyright 2010, 2011 Thomas McGuire <mcguire@kde.org>
  Copyright 2011 Roeland Jago Douma <unix@rullzer.com>
  Copyright 2012 Martin Klapetek <martin.klapetek@gmail.com>

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published
  by the Free Software Foundation; either version 2 of the License or
  ( at your option ) version 3 or, at the discretion of KDE e.V.
  ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "facebookresource.h"
#include "settings.h"
#include "settingsdialog.h"
#include "timestampattribute.h"
#ifdef HAVE_ACCOUNTS
#include "../shared/getcredentialsjob.h"
#endif

#include <libkfbapi/friendlistjob.h>
#include <libkfbapi/friendjob.h>
#include <libkfbapi/photojob.h>
#include <libkfbapi/alleventslistjob.h>
#include <libkfbapi/eventjob.h>
#include <libkfbapi/allnoteslistjob.h>
#include <libkfbapi/notejob.h>
#include <libkfbapi/noteaddjob.h>
#include <libkfbapi/facebookjobs.h>
#include <libkfbapi/postslistjob.h>
#include <libkfbapi/postjob.h>
#include <libkfbapi/notificationslistjob.h>
#include <libkfbapi/allpostslistjob.h>
#include <libkfbapi/postaddjob.h>

#include <Akonadi/AttributeFactory>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>
#include <akonadi/notes/noteutils.h> //krazy:exclude=camelcase wait for kdepimlibs 4.11

#include <Akonadi/SocialUtils/SocialNetworkAttributes>

using namespace Akonadi;

FacebookResource::FacebookResource( const QString &id )
    : ResourceBase( id )
{
  AttributeFactory::registerAttribute<TimeStampAttribute>();
  setNeedsNetwork( true );
  setObjectName( QLatin1String( "FacebookResource" ) );
  resetState();
  Settings::self()->setResourceId( identifier() );

  connect( this, SIGNAL(abortRequested()),
           this, SLOT(slotAbortRequested()) );
  connect( this, SIGNAL(reloadConfiguration()), SLOT(configurationChanged()) );

  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );

  AttributeFactory::registerAttribute<SocialNetworkAttributes>();
}

FacebookResource::~FacebookResource()
{
  Settings::self()->writeConfig();
}

void FacebookResource::configurationChanged()
{
#ifdef HAVE_ACCOUNTS
  if ( Settings::self()->accountId() ) {
    configureByAccount( Settings::self()->accountId() );
  }
#endif
  Settings::self()->writeConfig();
  synchronize();
}

#ifdef HAVE_ACCOUNTS
void FacebookResource::configureByAccount( int accountId )
{
  kDebug() << "Starting credentials job";
  GetCredentialsJob *gc = new GetCredentialsJob( accountId, this );
  connect(gc, SIGNAL(finished(KJob*)), SLOT(slotGetCredentials(KJob*)));
  gc->start();
}

void FacebookResource::slotGetCredentials(KJob *job)
{
  if (job->error()) {
    return;// We will get kDebug from within the job if it fails
  }

  GetCredentialsJob *gJob = qobject_cast<GetCredentialsJob*>(job);
  const QVariantMap data = gJob->credentialsData();
  const QString accessToken = data.value(QLatin1String("AccessToken")).toString();
  Settings::self()->setAccessToken(accessToken);

  synchronize();
}

#endif
void FacebookResource::aboutToQuit()
{
  slotAbortRequested();
}

void FacebookResource::abort()
{
  resetState();
  cancelTask();
}

void FacebookResource::abortWithError( const QString &errorMessage, bool authFailure )
{
  resetState();
  cancelTask( errorMessage );

  // This doesn't work, why?
  if ( authFailure ) {
    emit status( Broken, i18n( "Unable to login to Facebook, authentication failure." ) );
  }
}

void FacebookResource::resetState()
{
  mIdle = true;
  mNumFriends = -1;
  mNumPhotosFetched = 0;
  mCurrentJobs.clear();
  mExistingFriends.clear();
  mNewOrChangedFriends.clear();
  mPendingFriends.clear();
}

void FacebookResource::slotAbortRequested()
{
  if ( !mIdle ) {
    foreach ( const QPointer<KJob> &job, mCurrentJobs ) {
      kDebug() << "Killing current job:" << job;
      job->kill( KJob::Quietly );
    }
    abort();
  }
}

void FacebookResource::configure( WId windowId )
{
  const QPointer<SettingsDialog> settingsDialog( new SettingsDialog( this, windowId ) );
  if ( settingsDialog->exec() == QDialog::Accepted ) {
    emit configurationDialogAccepted();
  } else {
    emit configurationDialogRejected();
  }

  delete settingsDialog;
}

void FacebookResource::retrieveItems( const Akonadi::Collection &collection )
{
  Q_ASSERT( mIdle );

  if ( collection.remoteId() == friendsRID ) {
    mIdle = false;
    emit status( Running, i18n( "Preparing sync of friends list." ) );
    emit percent( 0 );
    ItemFetchJob * const fetchJob = new ItemFetchJob( collection );
    fetchJob->fetchScope().fetchAttribute<TimeStampAttribute>();
    fetchJob->fetchScope().fetchFullPayload( false );
    mCurrentJobs << fetchJob;
    connect( fetchJob, SIGNAL(result(KJob*)), this, SLOT(initialItemFetchFinished(KJob*)) );
  } else if ( collection.remoteId() == eventsRID ) {
    mIdle = false;
    emit status( Running, i18n( "Preparing sync of events list." ) );
    emit percent( 0 );
    KFbAPI::AllEventsListJob * const listJob =
      new KFbAPI::AllEventsListJob( Settings::self()->accessToken(), this );
    listJob->setLowerLimit( KDateTime::fromString( Settings::self()->lowerLimit(), QLatin1String("%Y-%m-%d") ) );
    mCurrentJobs << listJob;
    connect( listJob, SIGNAL(result(KJob*)), this, SLOT(eventListFetched(KJob*)) );
    listJob->start();
  } else if ( collection.remoteId() == notesRID ) {
    mIdle = false;
    emit status( Running, i18n( "Preparing sync of notes list." ) );
    emit percent( 0 );
    KFbAPI::AllNotesListJob * const notesJob =
      new KFbAPI::AllNotesListJob( Settings::self()->accessToken(), this );
    notesJob->setLowerLimit( KDateTime::fromString( Settings::self()->lowerLimit(), QLatin1String("%Y-%m-%d") ) );
    mCurrentJobs << notesJob;
    connect( notesJob, SIGNAL(result(KJob*)), this, SLOT(noteListFetched(KJob*)) );
    notesJob->start();
  } else if ( collection.remoteId() == postsRID ) {
    mIdle = false;
    emit status( Running, i18n( "Preparing sync of posts." ) );
    emit percent( 0 );
    KFbAPI::AllPostsListJob * const postsJob =
      new KFbAPI::AllPostsListJob( Settings::self()->accessToken(), this );
    postsJob->setLowerLimit(
      KDateTime::fromString(
        KDateTime::currentLocalDateTime().addDays( -3 ).toString(), QLatin1String("%Y-%m-%d") ) );
    mCurrentJobs << postsJob;
    connect( postsJob, SIGNAL(result(KJob*)), this, SLOT(postsListFetched(KJob*)) );
    postsJob->start();
  } else if ( collection.remoteId() == notificationsRID ) {
    mIdle = false;
    emit status( Running, i18n( "Preparing sync of notifications." ) );
    emit percent( 0 );
    KFbAPI::NotificationsListJob * const notificationsJob =
      new KFbAPI::NotificationsListJob( Settings::self()->accessToken(), this );
    mCurrentJobs << notificationsJob;
    connect( notificationsJob, SIGNAL(result(KJob*)), this, SLOT(notificationsListFetched(KJob*)) );
    notificationsJob->start();
  } else {
    // This can not happen
    Q_ASSERT( !"Unknown Collection" );
    cancelTask();
  }
}

bool FacebookResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );

  kDebug() << item.mimeType();

  if ( item.mimeType() == KABC::Addressee::mimeType() ) {
    // TODO: Is this ever called??
    mIdle = false;
    KFbAPI::FriendJob * const friendJob =
      new KFbAPI::FriendJob( item.remoteId(), Settings::self()->accessToken(), this );
    mCurrentJobs << friendJob;
    friendJob->setProperty( "Item", QVariant::fromValue( item ) );
    connect( friendJob, SIGNAL(result(KJob*)), this, SLOT(friendJobFinished(KJob*)) );
    friendJob->start();
  } else if ( item.mimeType() == Akonadi::NoteUtils::noteMimeType() ) {
    mIdle = false;
    KFbAPI::NoteJob * const noteJob =
      new KFbAPI::NoteJob( item.remoteId(), Settings::self()->accessToken(), this );
    mCurrentJobs << noteJob;
    noteJob->setProperty( "Item", QVariant::fromValue( item ) );
    connect( noteJob, SIGNAL(result(KJob*)), this, SLOT(noteJobFinished(KJob*)) );
    noteJob->start();
  } else if( item.mimeType() == QLatin1String("text/x-vnd.akonadi.socialfeeditem") ) {
    mIdle = false;
    KFbAPI::PostJob * const postJob =
      new KFbAPI::PostJob( item.remoteId(), Settings::self()->accessToken(), this );
    mCurrentJobs << postJob;
    postJob->setProperty( "Item", QVariant::fromValue( item ) );
    connect( postJob, SIGNAL(result(KJob*)), this, SLOT(postJobFinished(KJob*)) );
    postJob->start();
  } else if ( item.mimeType() == QLatin1String("text/x-vnd.akonadi.socialnotification") ) {
    //FIXME: Need to figure out how to fetch single notification
    kDebug() << "Notifications listjob";
  }
  return true;
}

void FacebookResource::retrieveCollections()
{
  Collection::List collections;
  if (Settings::self()->accountServices().contains(QLatin1String("facebook-contacts"))) {
    Collection friends;
    friends.setRemoteId( friendsRID );
    friends.setName( i18nc( "@title: addressbook name", "Friends on Facebook" ) );
    friends.setParentCollection( Akonadi::Collection::root() );
    friends.setContentMimeTypes( QStringList() << KABC::Addressee::mimeType() );
    friends.setRights( Collection::ReadOnly );
    EntityDisplayAttribute * const friendsDisplayAttribute = new EntityDisplayAttribute();
    friendsDisplayAttribute->setIconName( QLatin1String("facebookresource") );
    friends.addAttribute( friendsDisplayAttribute );
    collections << friends;
  }

  if (Settings::self()->accountServices().contains(QLatin1String("facebook-events"))) {
    Collection events;
    events.setRemoteId( eventsRID );
    events.setName( i18nc( "@title: events collection title", "Events on Facebook" ) );
    events.setParentCollection( Akonadi::Collection::root() );
    events.setContentMimeTypes( QStringList() << QLatin1String("text/calendar") << eventMimeType );
    events.setRights( Collection::ReadOnly );
    EntityDisplayAttribute * const evendDisplayAttribute = new EntityDisplayAttribute();
    evendDisplayAttribute->setIconName( QLatin1String("facebookresource") );
    events.addAttribute( evendDisplayAttribute );
    collections << events;
  }

  if (Settings::self()->accountServices().contains(QLatin1String("facebook-notes"))) {
    Collection notes;
    notes.setRemoteId( notesRID );
    notes.setName( i18nc( "@title: notes collection", "Notes on Facebook" ) );
    notes.setParentCollection( Akonadi::Collection::root() );
    notes.setContentMimeTypes( QStringList() << Akonadi::NoteUtils::noteMimeType() );
    notes.setRights( Collection::ReadOnly );
    EntityDisplayAttribute * const notesDisplayAttribute = new EntityDisplayAttribute();
    notesDisplayAttribute->setIconName( QLatin1String("facebookresource") );
    notes.addAttribute( notesDisplayAttribute );
    collections << notes;
  }

  if (Settings::self()->accountServices().contains(QLatin1String("facebook-feed"))) {
    Collection posts;
    posts.setRemoteId( postsRID );
    posts.setName( i18nc( "@title: posts collection", "Posts on Facebook" ) );
    posts.setParentCollection( Akonadi::Collection::root() );
    posts.setContentMimeTypes( QStringList() << QLatin1String("text/x-vnd.akonadi.socialfeeditem") );
    posts.setRights( Collection::CanCreateItem );
    EntityDisplayAttribute * const postsDisplayAttribute = new EntityDisplayAttribute();
    postsDisplayAttribute->setIconName( QLatin1String("facebookresource") );
    //facebook's max post length is 63206 as of September 2012
    //(Facebook ... Face Boo K ... hex(FACE) – K ... 64206 – 1000 = 63206)...don't ask me.
    SocialNetworkAttributes * const socialAttributes =
      new SocialNetworkAttributes( Settings::self()->userName(),
                                  QLatin1String( "Facebook" ),
                                  true,
                                  63206 );
    posts.addAttribute( postsDisplayAttribute );
    posts.addAttribute( socialAttributes );
    collections << posts;
  }

  if (Settings::self()->accountServices().contains(QLatin1String("facebook-notifications"))) {
    Collection notifications;
    notifications.setRemoteId( notificationsRID );
    notifications.setName( i18nc( "@title: notifications collection", "Notifications on Facebook" ) );
    notifications.setParentCollection( Akonadi::Collection::root() );
    notifications.setContentMimeTypes( QStringList() << QLatin1String("text/x-vnd.akonadi.socialnotification") );
    notifications.setRights( Collection::ReadOnly );
    EntityDisplayAttribute * const notificationsDisplayAttribute = new EntityDisplayAttribute();
    notificationsDisplayAttribute->setIconName( QLatin1String("facebookresource") );
    notifications.addAttribute( notificationsDisplayAttribute );
    collections << notifications;
  }

  collectionsRetrieved( collections );
}

void FacebookResource::itemRemoved( const Akonadi::Item &item )
{
  if ( item.mimeType() == QLatin1String("text/x-vnd.akonadi.note") ) {
    mIdle = false;
    KFbAPI::FacebookDeleteJob * const deleteJob =
      new KFbAPI::FacebookDeleteJob( item.remoteId(),
                                     Settings::self()->accessToken(),
                                     this );
    mCurrentJobs << deleteJob;
    deleteJob->setProperty( "Item", QVariant::fromValue( item ) );
    connect( deleteJob, SIGNAL(result(KJob*)), this, SLOT(deleteJobFinished(KJob*)) );
    deleteJob->start();
  } else {
    // Shouldn't happen, all other items are read-only
    Q_ASSERT( !"Unable to delete item, not ours." );
    cancelTask();
  }
}

void FacebookResource::deleteJobFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  mCurrentJobs.removeAll( job );
  if ( job->error() ) {
    abortWithError( i18n( "Unable to delete note from server: %1", job->errorText() ) );
  } else {
    const Item item = job->property( "Item" ).value<Item>();
    changeCommitted( item );
    resetState();
  }
}

void FacebookResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  if ( collection.remoteId() == notesRID ) {
    if ( item.hasPayload<KMime::Message::Ptr>() ) {
      const KMime::Message::Ptr note = item.payload<KMime::Message::Ptr>();
      const QString subject = note->subject()->asUnicodeString();
      const QString message = QString::fromUtf8(note->body());

      mIdle = false;
      KFbAPI::NoteAddJob * const addJob =
        new KFbAPI::NoteAddJob( subject, message, Settings::self()->accessToken(), this );
      mCurrentJobs << addJob;
      addJob->setProperty( "Item", QVariant::fromValue( item ) );
      connect( addJob, SIGNAL(result(KJob*)), this, SLOT(noteAddJobFinished(KJob*)) );
      addJob->start();
    } else {
      Q_ASSERT( !"Note has wrong mimetype." );
      cancelTask();
    }
  } else if ( collection.remoteId() == postsRID ) {
    kDebug() << "Adding new status";
    QString message;
    if ( item.hasPayload<KFbAPI::PostInfo>() ) {
      message = item.payload<KFbAPI::PostInfo>().message();
      mIdle = false;
    } else if ( item.hasPayload<Akonadi::SocialFeedItem>() ) {
      message = item.payload<Akonadi::SocialFeedItem>().postText();
      mIdle = false;
    }

    KFbAPI::PostAddJob * const addJob =
      new KFbAPI::PostAddJob( message, Settings::self()->accessToken(), this );
    mCurrentJobs << addJob;
    addJob->setProperty( "Item", QVariant::fromValue( item ) );
    connect( addJob, SIGNAL(result(KJob*)), this, SLOT(postAddJobFinished(KJob*)) );
    addJob->start();

  } else {
    Q_ASSERT( !"Can not add this type of item!" );
    cancelTask();
  }
}

AKONADI_RESOURCE_MAIN( FacebookResource )
