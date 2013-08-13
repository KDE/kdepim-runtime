/*
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
#include "timestampattribute.h"

#include <libkfbapi/postjob.h>
#include <libkfbapi/notificationslistjob.h>
#include <libkfbapi/notificationinfo.h>
#include <libkfbapi/notificationsmarkreadjob.h>

#include <Akonadi/AttributeFactory>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionFetchJob>

#include <KLocalizedString>
#include <KNotification>
#include <KAboutData>
#include <KStatusNotifierItem>
#include <KMenu>
#include <KToolInvocation>

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

    //clear any notifications cached for display
    mDisplayedNotifications.clear();

    KSharedConfigPtr knotificationHistory = KSharedConfig::openConfig(QLatin1String("facebook-notificationsrc"));

    Q_FOREACH ( const KFbAPI::NotificationInfo &notificationInfo, listJob->notifications() ) {
      Item notification;
      notification.setRemoteId( notificationInfo.id() );
      notification.setMimeType( QLatin1String("text/x-vnd.akonadi.socialnotification") );
      notification.setPayload<KFbAPI::NotificationInfo>( notificationInfo );
      notificationItems.append( notification );

      if (Settings::self()->displayNotifications()) {
        if (notificationInfo.unread()) {
            mDisplayedNotifications.append(notificationInfo);
        } else {
            //if the notification is marked as read, remove it from saved notifications
            //to keep it small
            knotificationHistory->deleteGroup(notificationInfo.id());
        }
      }
    }

    if (Settings::self()->displayNotifications()) {
        displayNotificationsToUser(FacebookResource::KSNIandKNotification);
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

void FacebookResource::displayNotificationsToUser(FbNotificationPresentation displayType)
{
    if (mDisplayedNotifications.isEmpty()) {
        //if we have no notifications to process, quit
        if (!mNotificationSNI.isNull()) {
            //also check if the SNI exists, destroy if it is
            mNotificationSNI.data()->deleteLater();
        }
        return;
    }

    if (mNotificationSNI.isNull()) {
        mNotificationSNI = new KStatusNotifierItem(this);
        mNotificationSNI->setCategory(KStatusNotifierItem::Communications);
        mNotificationSNI->setIconByName(QLatin1String("facebookresource"));
        mNotificationSNI->setAttentionIconByName(QLatin1String("facebookresource"));
        mNotificationSNI->setStandardActionsEnabled(false);

        connect(mNotificationSNI.data(), SIGNAL(activateRequested(bool,QPoint)),
                this, SLOT(notificationSNIActivated(bool,QPoint)));
    }

    mNotificationSNI->setContextMenu(0);

    //the menu is deleted by SNI whenever we set new context menu
    //and/or when the SNI is destroyed
    KMenu *contextMenu = new KMenu();
    contextMenu->addTitle(i18n("Facebook Notifications"));

    //use separate config file for the displayed notifications
    KSharedConfigPtr knotificationHistory = KSharedConfig::openConfig(QLatin1String("facebook-notificationsrc"));

    //the logic here is - we want to display KNotification for each batch of Facebook notifications
    //only once (KNotification will just show the newest not-yet-displayed with "...and N more" appended),
    //but on the SNI tooltip we want to show always the newest three, so different strings must be constructed
    int tooltipCount = 0;
    int notificationCount = 0;

    QString sniTooltip = QLatin1String("<ul>");
    QString notificationString;

    Q_FOREACH (const KFbAPI::NotificationInfo &notification, mDisplayedNotifications) {

        //**** Process KNotification first

        //The KNotification plasmoid shows max 4 lines of text, so unfortunately we need to limit this
        //to only one notification being displayed in the notification and then add the "...and N more"
        //but this "N more" means "N more notifications that were not displayed before",
        //so we need to count how many new notifications we got using the notificationCount var
        if (displayType == FacebookResource::KSNIandKNotification) {
            KConfigGroup notificationConfig = knotificationHistory->group(notification.id());

            //this is true if this notification was already in KNotification before
            bool skip = false;

            //if we don't have any Facebook notifications to show yet
            if (notificationString.isEmpty()) {

                //check if we have stored config for this notification
                if (notificationConfig.isValid()) {
                    //now check if the stored notification has changed or not
                    if (notificationConfig.readEntry(QLatin1String("updatedTime")) == notification.updatedTimeString()) {
                        //if it was not changed, just skip it
                        skip = true;
                    }
                }

                //if the config group does not exist, skip == false and we continue
                if (!skip) {
                    notificationString.append(notification.title());
                    notificationString.append(QLatin1String("\n"));

                    notificationCount++;
                    notificationConfig.writeEntry(QLatin1String("updatedTime"), notification.updatedTimeString());
                } else {
                    kDebug() << "Skipping notification" << notification.id();
                }
            }
        }

        //**** Process KStatusNotifierItem entries

        //include only up to 3 notifications in the KNotification/SNI tooltip
        if (tooltipCount < 3) {
            sniTooltip.append(QString::fromLatin1("<li>%1</li>").arg(notification.title()));
            tooltipCount++;
        }

        //create for each notification a qaction to put in SNI;
        //set the link as property for fast access to it, id is used to remove it
        //from the SNI menu
        QAction *action = new QAction(notification.title().simplified(), contextMenu);
        action->setProperty("notificationLink", notification.link());
        action->setProperty("notificationId", notification.id());
        action->setToolTip(i18nc("@info:tooltip", "Open browser"));
        connect(action, SIGNAL(triggered(bool)),
                this, SLOT(notificationLinkActivated()));
        contextMenu->addAction(action);
    }

    sniTooltip = sniTooltip.append(QLatin1String("</ul>"));

    notificationString.append(i18ncp("This string is appended to a Facebook notification displayed "
                                     "in KNotification, indicating how many more notifications "
                                     "were received, but are not displayed. So the KNotification "
                                     "meaning will be 'You have X new Facebook notifications, here's the "
                                     "first one and then there are Y more",
                                     "...and one more",
                                     "...and %1 more",
                                     mDisplayedNotifications.size() - notificationCount));

    //save which notifications are being displayed in KNotification
    knotificationHistory->sync();

    contextMenu->addSeparator();

    QAction *dismissAll = new QAction(i18n("Mark all notifications as read"), contextMenu);
    connect(dismissAll, SIGNAL(triggered(bool)),
            this, SLOT(markNotificationsAsReadTriggered()));

    contextMenu->addAction(dismissAll);

    QAction *hideSNI = new QAction(i18n("Hide"), contextMenu);
    connect(hideSNI, SIGNAL(triggered(bool)),
            this, SLOT(hideNotificationsSNITriggered()));

    contextMenu->addAction(hideSNI);

    //if we have more than 3 notifications, append "...and N more" at the end of the string
    if (mDisplayedNotifications.size() > 3) {
        sniTooltip.append(i18ncp("This string is appended at the end of Facebook notifications "
                                        "displayed in the system notifications, indicating there are more "
                                        "notifications, which are not displayed.",
                                 "...and 1 more",
                                 "...and %1 more",
                                 mDisplayedNotifications.size() - 3));
    }

    if (displayType == FacebookResource::KSNIandKNotification && !notificationString.isEmpty()) {
        //create the system notification
        KNotification *notification = new KNotification(QLatin1String("facebookNotification"), KNotification::CloseOnTimeout);
        notification->setTitle(i18np("New Facebook Notification",
                                     "%1 New Facebook Notifications",
                                     mDisplayedNotifications.size()));
        notification->setText(notificationString);
        //we need to manually create the KAboutData because KGlobal::mainComponent() returns
        //the name with appended resource id, ie "akonadi_facebook_resource_72"
        //this in turn causes KNotification to not find the .notifyrc file
        //so we must set it manually to the name of the .notifyrc file
        KAboutData aboutData("akonadi_facebook_resource", 0, KLocalizedString(), 0);
        notification->setComponentData(KComponentData(aboutData));
        notification->sendEvent();
    }

    mNotificationSNI->setStatus(KStatusNotifierItem::NeedsAttention);
    mNotificationSNI->setContextMenu(contextMenu);

    //if we're handling just one notification, show its text directly in the tooltip,
    //otherwise show the tooltip string built above
    if (mDisplayedNotifications.size() == 1) {
        mNotificationSNI->setProperty("notificationLink", mDisplayedNotifications.first().link());
        mNotificationSNI->setToolTip(QLatin1String("facebookresource"),
                                            mDisplayedNotifications.first().title(),
                                            QString());
    } else {
        mNotificationSNI->setToolTip(QLatin1String("facebookresource"),
                                     i18np("You have one new Facebook notification",
                                           "You have %1 new Facebook notifications",
                                           mDisplayedNotifications.size()),
                                     sniTooltip);
    }
}

void FacebookResource::notificationSNIActivated(bool active, const QPoint &point)
{
    Q_UNUSED(active);

    KStatusNotifierItem *sni = qobject_cast<KStatusNotifierItem*>(sender());

    Q_ASSERT(sni);
    if (!sni) {
        return;
    }

    //this property is set only in case we're handling single notification
    QUrl link = sni->property("notificationLink").toUrl();

    if (link.isEmpty()) {
        //this^ means we have multiple notifications and we should show the menu
        sni->contextMenu()->popup(point);
        //The SNI is not deleted in this branch, here's what happens in here:
        // - the context menu opens
        // - user clicks one of the notifications
        // - this notification is removed from the cached list (mDisplayedNotifications)
        // - displayNotificationsToUser(..) is called
        // - this checks if mDisplayedNotifications is empty, if yes, the SNI is deleted.
    } else {
        //if we're handling single notification, open the browser and destroy the SNI as it's useful no more
        KToolInvocation::invokeBrowser(link.toString());
        sni->deleteLater();
    }
}

void FacebookResource::notificationLinkActivated()
{
    QUrl link = sender()->property("notificationLink").toUrl();
    QString id = sender()->property("notificationId").toString();

    //remove the notification from cached notifications
    //so it does not reappear in the menu
    QMutableListIterator<KFbAPI::NotificationInfo> i(mDisplayedNotifications);
    while (i.hasNext()) {
        if (i.next().id() == id) {
            i.remove();
        }
    }

    //open the browser with the link
    if (!link.isEmpty()) {
        KToolInvocation::invokeBrowser(link.toString());
    }

    //mark the notification as read on the server
    KFbAPI::NotificationsMarkReadJob * const markNotificationJob =
        new KFbAPI::NotificationsMarkReadJob(id, Settings::self()->accessToken(), this);

    mCurrentJobs << markNotificationJob;
    connect(markNotificationJob, SIGNAL(result(KJob*)),
            this, SLOT(notificationMarkAsReadJobFinished(KJob*)));

    markNotificationJob->start();

    //trigger reupdate of the SNI
    displayNotificationsToUser(FacebookResource::KSNIonly);
}

void FacebookResource::notificationMarkAsReadJobFinished(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorString() << job->errorText();
    }

    mCurrentJobs.removeAll( job );

    //only refetch the notifications if there are no more jobs running
    if (mCurrentJobs.isEmpty()) {
        //refetch notifications
        Collection notifications;
        notifications.setRemoteId(notificationsRID);
        CollectionFetchJob *collectionJob = new CollectionFetchJob(notifications, CollectionFetchJob::Base, this);

        connect(collectionJob, SIGNAL(result(KJob*)),
                this, SLOT(notificationCollectionFetchJobFinished(KJob*)));
    }

    job->deleteLater();
}

void FacebookResource::notificationCollectionFetchJobFinished(KJob *job)
{
    CollectionFetchJob *collectionJob = qobject_cast<CollectionFetchJob*>(job);

    Q_FOREACH(const Akonadi::Collection &collection, collectionJob->collections()) {
        if (collection.remoteId() == notificationsRID) {
            synchronizeCollection(collection.id());
            //there is only one notifications collection, bail out if it was found
            break;
        }
    }
}

void FacebookResource::markNotificationsAsReadTriggered()
{
    for (int i = 0; i < mDisplayedNotifications.size(); i++) {
        KFbAPI::NotificationsMarkReadJob * const markNotificationJob =
        new KFbAPI::NotificationsMarkReadJob(mDisplayedNotifications.at(i).id(),
                                             Settings::self()->accessToken(),
                                             this);

        mCurrentJobs << markNotificationJob;
        connect(markNotificationJob, SIGNAL(result(KJob*)),
                this, SLOT(notificationMarkAsReadJobFinished(KJob*)));

        markNotificationJob->start();
    }
}

void FacebookResource::hideNotificationsSNITriggered()
{
    Q_ASSERT(!mNotificationSNI.isNull());

    if (!mNotificationSNI.isNull()) {
        mNotificationSNI->setStatus(KStatusNotifierItem::Passive);
    }
}
