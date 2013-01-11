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
#ifndef FACEBOOKRESOURCE_H
#define FACEBOOKRESOURCE_H

#include <libkfbapi/userinfo.h>
#include <libkfbapi/postinfo.h>

#include <Akonadi/SocialUtils/SocialFeedItem>

#include <Akonadi/ResourceBase>
#include <QPointer>
#include <QMutex>

class FacebookResource : public Akonadi::ResourceBase,
                         public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    explicit FacebookResource( const QString &id );
    ~FacebookResource();

    using ResourceBase::synchronize;

  public Q_SLOTS:
    void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

    void itemRemoved( const Akonadi::Item &item );
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );

  protected:
    void aboutToQuit();

  private Q_SLOTS:

    void slotAbortRequested();
    void configurationChanged();
    void friendListJobFinished( KJob *job );
    void friendJobFinished( KJob *job );
    void photoJobFinished( KJob *job );
    void detailedFriendListJobFinished( KJob *job );
    void initialItemFetchFinished( KJob *job );
    void eventListFetched( KJob *job );
    void detailedEventListJobFinished( KJob *job );
    void noteListFetched( KJob *job );
    void noteJobFinished( KJob *job );
    void noteAddJobFinished( KJob *job );
    void postJobFinished( KJob *job );
    void postsListFetched( KJob *job );
    void postAddJobFinished( KJob *job );
    void notificationsListFetched( KJob *job );

    void deleteJobFinished( KJob *job );

  private:
    void fetchPhotos();
    void resetState();
    void abortWithError( const QString &errorMessage, bool authFailure = false );
    void abort();

    enum FormattingStringType {
      FacebookComment = 0,
      FacebookLike = 1
    };
    QString formatI18nString( FormattingStringType type, int n );

    void fetchNewOrChangedFriends();
    void finishFriendFetching();
    void finishEventsFetching();
    void finishNotesFetching();
    void finishPostsFetching();
    void finishNotificationsFetching();
    Akonadi::SocialFeedItem convertToSocialFeedItem( const KFbAPI::PostInfo &postinfo );

    // Friends that are already stored on the Akonadi server
    QMap<QString, KDateTime> mExistingFriends;

    // Pending new/changed friends we still need to download
    QList<KFbAPI::UserInfo> mPendingFriends;

    QList<KFbAPI::UserInfo> mNewOrChangedFriends;

    // Total number of new & changed friends
    int mNumFriends;
    int mNumPhotosFetched;

    bool mIdle;
    QList< QPointer<KJob> > mCurrentJobs;
};

#endif
