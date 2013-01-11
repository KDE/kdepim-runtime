/* Copyright 2012 Martin Klapetek <martin.klapetek@gmail.com>

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
#include "timestampattribute.h"

#include <libkfbapi/postjob.h>
#include <libkfbapi/notificationslistjob.h>
#include <libkfbapi/notificationinfo.h>

#include <Akonadi/AttributeFactory>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>

#include <KLocalizedString>

using namespace Akonadi;

void FacebookResource::notificationsListFetched( KJob *job )
{
  Q_ASSERT( !mIdle );
  KFbAPI::NotificationsListJob * const listJob = dynamic_cast<KFbAPI::NotificationsListJob*>( job );
  Q_ASSERT( listJob );
  mCurrentJobs.removeAll( job );

  if ( listJob->error() ) {
    abortWithError( i18n( "Unable to get notifications from server: %1", listJob->errorString() ),
                    listJob->error() == KFbAPI::FacebookJob::AuthenticationProblem );
  } else {
    setItemStreamingEnabled( true );

    Item::List notificationItems;
    kDebug() << "Going into foreach";
    Q_FOREACH ( const KFbAPI::NotificationInfo &notificationInfo, listJob->notifications() ) {
      Item notification;
      notification.setRemoteId( notificationInfo.id() );
      notification.setMimeType( "text/x-vnd.akonadi.socialnotification" );
      notification.setPayload<KFbAPI::NotificationInfo>( notificationInfo );
      notificationItems.append( notification );;
    }

    itemsRetrieved( notificationItems );
    itemsRetrievalDone();
    finishNotificationsFetching();
  }

  listJob->deleteLater();
}

void FacebookResource::finishNotificationsFetching()
{
  emit percent( 100 );
  emit status( Idle, i18n( "All notifications fetched from server." ) );
  resetState();
}
