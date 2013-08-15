/*
  Copyright (C) 2012  Martin Klapetek <martin.klapetek@gmail.com>

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

#include <libkfbapi/allpostslistjob.h>
#include <libkfbapi/postslistjob.h>
#include <libkfbapi/noteaddjob.h>
#include <libkfbapi/postjob.h>
#include <libkfbapi/postaddjob.h>

#include <Akonadi/AttributeFactory>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>

using namespace Akonadi;

void FacebookResource::postsListFetched( KJob *job )
{
  Q_ASSERT( !mIdle );
  KFbAPI::AllPostsListJob * const listJob = dynamic_cast<KFbAPI::AllPostsListJob*>( job );
  Q_ASSERT( listJob );
  mCurrentJobs.removeAll( job );

  if ( listJob->error() ) {
    abortWithError( i18n( "Unable to get posts from server: %1", listJob->errorString() ),
                    listJob->error() == KFbAPI::FacebookJob::AuthenticationProblem );
  } else {
    setItemStreamingEnabled( true );

    Item::List postItems;
    kDebug() << "Going into foreach";
    Q_FOREACH ( const KFbAPI::PostInfo &postInfo, listJob->allPosts() ) {
      Item post;
      post.setRemoteId( postInfo.id() );
      post.setMimeType( QLatin1String("text/x-vnd.akonadi.socialfeeditem") );
      post.setPayload<Akonadi::SocialFeedItem>( convertToSocialFeedItem( postInfo ) );
      postItems.append( post );
    }

    itemsRetrieved( postItems );
    itemsRetrievalDone();
    finishPostsFetching();
  }

  listJob->deleteLater();
}

void FacebookResource::finishPostsFetching()
{
  emit percent( 100 );
  emit status( Idle, i18n( "All posts fetched from server." ) );
  resetState();
}

void FacebookResource::postJobFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  KFbAPI::PostJob * const postJob = dynamic_cast<KFbAPI::PostJob*>( job );
  Q_ASSERT( postJob );
  Q_ASSERT( postJob->postInfo().size() == 1 );
  mCurrentJobs.removeAll( job );

  if ( postJob->error() ) {
    abortWithError( i18n( "Unable to get information about post from server: %1",
                          postJob->errorText() ) );
  } else {
    Item post = postJob->property( "Item" ).value<Item>();
    post.setPayload( convertToSocialFeedItem( postJob->postInfo().first() ) );
    itemRetrieved( post );
    resetState();
  }

  postJob->deleteLater();
}

SocialFeedItem FacebookResource::convertToSocialFeedItem( const KFbAPI::PostInfo &postinfo )
{
  SocialFeedItem item;
  item.setPostId( postinfo.id() );
  item.setPostText( postinfo.message().isEmpty() ? postinfo.story() : postinfo.message() );
  item.setPostLink( postinfo.link() );
  item.setPostLinkTitle( postinfo.name() );
  item.setPostImageUrl( postinfo.pictureUrl() );
  item.setPostTime( postinfo.createdTimeString(), QLatin1String( "%Y-%m-%dT%H:%M:%S%z" ) );

  QString infoString;

  qlonglong commentsCount = postinfo.commentsMap().value( QLatin1String("count") ).toLongLong();
  qlonglong likesCount = postinfo.likesMap().value( QLatin1String("count") ).toLongLong();

  if ( commentsCount > 0 && likesCount > 0 ) {
    infoString =
      i18nc( "This is a string putting together two previously translated strings, "
             "resulting form is eg '1 comment, 3 likes'. "
             "Main purpose of this is to give the ability to change the "
             "comma delimiter to something more appropriate for some languages.",
             "%1, %2",
             formatI18nString( FacebookResource::FacebookComment, commentsCount ),
             formatI18nString( FacebookResource::FacebookLike, likesCount ) );
  } else if ( commentsCount > 0 && likesCount == 0 ) {
    infoString = formatI18nString( FacebookResource::FacebookComment, commentsCount );
  } else if ( commentsCount == 0 && likesCount > 0 ) {
    infoString = formatI18nString( FacebookResource::FacebookLike, likesCount );
  }

//     QList<PostReply> replies;
//
//     Q_FOREACH ( const QVariant &listItem, postinfo.commentsMap().value( "data" ).toList() ) {
//       QVariantMap listItemMap = listItem.toMap();
//       Akonadi::PostReply postReply;
//       postReply.replyText = listItemMap.value( "message" ).toString();
//       postReply.replyId = listItemMap.value( "id" ).toString();
//       postReply.postId = postinfo.id();
//       postReply.userId = listItemMap.value( "from" ).toMap().value( "id" ).toString();
//       postReply.userName = listItemMap.value( "from" ).toMap().value( "name" ).toString();
//
//       replies.append( postReply );
//     }
//
//     item.setPostReplies( replies );

  item.setPostInfo( infoString );

  KFbAPI::UserInfo user = postinfo.from();

  item.setUserId( user.id() );
  if ( user.username().isEmpty() ) {
    item.setUserName( user.id() );
  } else {
    item.setUserName( user.username() );
  }
  item.setUserDisplayName( user.name() );
  item.setNetworkString(
    i18nc( "This string is used in a sentence "
           "'Some Name on Facebook: Just had lunch.', "
           "so should be translated in such form. "
           "This string is defined by the resource and "
           "the whole sentence is composed in the UI.",
           "on Facebook" ) );
  item.setAvatarUrl(
    QString::fromLatin1( "https://graph.facebook.com/%1/picture?type=square" ).arg( user.id() ) );

//  item.setItemSourceMap( QJson::QObjectHelper::qobject2qvariant( postinfo.data() ) );

  return item;
}

void FacebookResource::postAddJobFinished( KJob *job )
{
  Q_ASSERT( !mIdle );
  Q_ASSERT( mCurrentJobs.indexOf( job ) != -1 );
  KFbAPI::PostAddJob * const addJob = dynamic_cast<KFbAPI::PostAddJob*>( job );
  Q_ASSERT( addJob );
  mCurrentJobs.removeAll( job );

  if ( job->error() ) {
    abortWithError( i18n( "Unable to post the status to server: %1", job->errorText() ) );
  } else {
    Item post = addJob->property( "Item" ).value<Item>();
    //we fill in a random fake id to prevent duplicates - this post would be in the collection twice
    //once the resource syncs again, filling a random id guarantees that this Item will be removed
    //with the next sync and will be replaced by the real item from the server
    post.setRemoteId( QLatin1String("non-existing-id") );
    changeCommitted( post );
    resetState();
    kDebug() << "Status posted to server";
  }

  addJob->deleteLater();
}

QString FacebookResource::formatI18nString( FormattingStringType type, int n )
{
  switch ( type ) {
  case FacebookResource::FacebookComment:
    return i18ncp( "Text denoting how many comments given post have",
                   "1 comment", "%1 comments", n );
  case FacebookResource::FacebookLike:
    return i18ncp( "Text denoting how many 'facebook likes' given post have",
                   "1 like", "%1 likes", n );
  }

  return QString();
}
