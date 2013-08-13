/*
  Copyright 2010, 2011 Thomas McGuire <mcguire@kde.org>
  Copyright 2011 Roeland Jago Douma <unix@rullzer.com>

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

#include <libkfbapi/friendlistjob.h>
#include <libkfbapi/friendjob.h>
#include <libkfbapi/photojob.h>

#include <Akonadi/AttributeFactory>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>

using namespace Akonadi;

void FacebookResource::initialItemFetchFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  ItemFetchJob * const itemFetchJob = dynamic_cast<ItemFetchJob*>( job );
  Q_ASSERT( itemFetchJob );
  mCurrentJobs.removeAll( job );

  if ( itemFetchJob->error() ) {
    abortWithError( i18n( "Unable to get list of existing friends from the Akonadi server: %1",
                          itemFetchJob->errorString() ) );
  } else {
    foreach ( const Item &item, itemFetchJob->items() ) {
      if ( item.hasAttribute<TimeStampAttribute>() ) {
        mExistingFriends.insert( item.remoteId(),
                                 item.attribute<TimeStampAttribute>()->timeStamp() );
      }
    }

    setItemStreamingEnabled( true );
    KFbAPI::FriendListJob * const friendListJob =
      new KFbAPI::FriendListJob( Settings::self()->accessToken(), this );
    mCurrentJobs << friendListJob;
    connect( friendListJob, SIGNAL(result(KJob*)), this, SLOT(friendListJobFinished(KJob*)) );
    emit status( Running, i18n( "Retrieving friends list." ) );
    emit percent( 2 );
    friendListJob->start();
  }

  itemFetchJob->deleteLater();
}

void FacebookResource::friendListJobFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  KFbAPI::FriendListJob * const friendListJob = dynamic_cast<KFbAPI::FriendListJob*>( job );
  Q_ASSERT( friendListJob );
  mCurrentJobs.removeAll( job );

  if ( friendListJob->error() ) {
    abortWithError( i18n( "Unable to get list of friends from server: %1",
                          friendListJob->errorText() ),
                    friendListJob->error() == KFbAPI::FacebookJob::AuthenticationProblem );
  } else {

    // Figure out which items are new or changed
    foreach ( const KFbAPI::UserInfo &user, friendListJob->friends() ) {
#if 0 // Bah, Facebook's timestamp doesn't seem to get updated when a user's profile changes :(
      // See http://bugs.developers.facebook.net/show_bug.cgi?id=15475
      const KDateTime stampOfExisting = mExistingFriends.value( user->id(), KDateTime() );
      if ( !stampOfExisting.isValid() ) {
        kDebug() << "Friend" << user->id() << user->name() << "is new!";
        mNewOrChangedFriends.append( user );
      } else if ( user->updatedTime() > stampOfExisting ) {
        kDebug() << "Friend" << user->id() << user->name() << "is updated!";
        mNewOrChangedFriends.append( user );
      } else {
        //kDebug() << "Friend" << user->id() << user->name() << "is old.";
      }
#else
      mNewOrChangedFriends.append( user );
#endif
    }

    // Delete items that are in the Akonadi DB but no on FB
    Item::List removedItems;
    foreach ( const QString &friendId, mExistingFriends.keys() ) { //krazy:exclude=foreach
      bool found = false;
      foreach ( const KFbAPI::UserInfo &user, friendListJob->friends() ) {
        if ( user.id() == friendId ) {
          found = true;
          break;
        }
      }
      if ( !found ) {
        kDebug() << friendId << "is no longer your friend :(";
        Item removedItem;
        removedItem.setRemoteId( friendId );
        removedItems.append( removedItem );
      }
    }
    itemsRetrievedIncremental( Item::List(), removedItems );

    if ( mNewOrChangedFriends.isEmpty() ) {
      finishFriendFetching();
    } else {
      emit status( Running, i18n( "Retrieving friends' details." ) );
      emit percent( 5 );
      fetchNewOrChangedFriends();
    }
  }

  friendListJob->deleteLater();
}

void FacebookResource::fetchNewOrChangedFriends()
{
  QStringList mewOrChangedFriendIds;
  Q_FOREACH ( const KFbAPI::UserInfo &user, mNewOrChangedFriends ) {
    mewOrChangedFriendIds.append( user.id() );
  }
  KFbAPI::FriendJob * const friendJob =
    new KFbAPI::FriendJob( mewOrChangedFriendIds, Settings::self()->accessToken(), this );
  mCurrentJobs << friendJob;
  connect( friendJob, SIGNAL(result(KJob*)), this, SLOT(detailedFriendListJobFinished(KJob*)) );
  friendJob->start();
}

void FacebookResource::detailedFriendListJobFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  KFbAPI::FriendJob * const friendJob = dynamic_cast<KFbAPI::FriendJob*>( job );
  Q_ASSERT( friendJob );
  mCurrentJobs.removeAll( job );

  if ( friendJob->error() ) {
    abortWithError( i18n( "Unable to retrieve friends' information from server: %1",
                          friendJob->errorText() ) );
  } else {
    mPendingFriends = friendJob->friendInfo();
    mNumFriends = mPendingFriends.size();
    emit status( Running, i18n( "Retrieving friends' photos." ) );
    emit percent( 10 );
    fetchPhotos();
  }

  friendJob->deleteLater();
}

void FacebookResource::fetchPhotos()
{
  mIdle = false;
  mNumPhotosFetched = 0;
  Q_FOREACH ( const KFbAPI::UserInfo &f, mPendingFriends ) {
    KFbAPI::PhotoJob * const photoJob =
      new KFbAPI::PhotoJob( f.id(), Settings::self()->accessToken(), this );
    mCurrentJobs << photoJob;
    photoJob->setProperty( "friend", QVariant::fromValue( f ) );
    connect( photoJob, SIGNAL(result(KJob*)), this, SLOT(photoJobFinished(KJob*)) );
    photoJob->start();
  }
}

void FacebookResource::finishFriendFetching()
{
  Q_ASSERT( mCurrentJobs.size() == 0 );

  mPendingFriends.clear();
  emit percent( 100 );
  if ( mNumFriends > 0 ) {
    emit status( Idle, i18np( "Updated one friend from the server.",
                              "Updated %1 friends from the server.", mNumFriends ) );
  } else {
    emit status( Idle, i18n( "Finished, no friends needed to be updated." ) );
  }
  resetState();
}

void FacebookResource::photoJobFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  KFbAPI::PhotoJob * const photoJob = dynamic_cast<KFbAPI::PhotoJob*>( job );
  Q_ASSERT( photoJob );
  const KFbAPI::UserInfo user = job->property( "friend" ).value<KFbAPI::UserInfo>();
  mCurrentJobs.removeOne( job );

  if ( photoJob->error() ) {
    abortWithError( i18n( "Unable to retrieve friends' photo from server: %1",
                          photoJob->errorText() ) );
  } else {
    // Create Item
    KABC::Addressee addressee = user.toAddressee();
    addressee.setPhoto( KABC::Picture( photoJob->photo() ) );
    Item newUser;
    newUser.setRemoteId( user.id() );
    newUser.setMimeType( QLatin1String("text/directory") );
    newUser.setPayload<KABC::Addressee>( addressee );
    TimeStampAttribute * const timeStampAttribute = new TimeStampAttribute();
    timeStampAttribute->setTimeStamp( user.updatedTime() );
    newUser.addAttribute( timeStampAttribute );

    itemsRetrievedIncremental( Item::List() << newUser, Item::List() );
    mNumPhotosFetched++;

    if ( mNumPhotosFetched != mNumFriends ) {
      const int alreadyDownloadedFriends = mNumPhotosFetched;
      const float percentageDone = alreadyDownloadedFriends / ( float )mNumFriends * 100.0f;
      emit percent( 10 + percentageDone * 0.9f );
    } else {
      itemsRetrievalDone();
      finishFriendFetching();
    }
  }

  photoJob->deleteLater();
}

void FacebookResource::friendJobFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  KFbAPI::FriendJob * const friendJob = dynamic_cast<KFbAPI::FriendJob*>( job );
  Q_ASSERT( friendJob );
  Q_ASSERT( friendJob->friendInfo().size() == 1 );
  mCurrentJobs.removeAll( job );

  if ( friendJob->error() ) {
    abortWithError( i18n( "Unable to get information about friend from server: %1",
                          friendJob->errorText() ) );
  } else {
    Item user = friendJob->property( "Item" ).value<Item>();
    user.setPayload<KABC::Addressee>( friendJob->friendInfo().first().toAddressee() );
    // TODO: What about picture here?
    itemRetrieved( user );
    resetState();
  }

  friendJob->deleteLater();
}
