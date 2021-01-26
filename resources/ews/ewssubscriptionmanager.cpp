/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewssubscriptionmanager.h"

#include "ewsgeteventsrequest.h"
#include "ewsgetfolderrequest.h"
#include "ewsgetstreamingeventsrequest.h"
#include "ewsresource_debug.h"
#include "ewssettings.h"
#include "ewssubscribedfoldersjob.h"
#include "ewssubscriberequest.h"
#include "ewsunsubscriberequest.h"
#include <QPointer>

// TODO: Allow customization
static Q_CONSTEXPR uint pollInterval = 10; /* seconds */

static Q_CONSTEXPR uint streamingTimeout = 30; /* minutes */

static Q_CONSTEXPR uint streamingConnTimeout = 60; /* seconds */

EwsSubscriptionManager::EwsSubscriptionManager(EwsClient &client, const EwsId &rootId, EwsSettings *settings, QObject *parent)
    : QObject(parent)
    , mEwsClient(client)
    , mPollTimer(this)
    , mMsgRootId(rootId)
    , mFolderTreeChanged(false)
    , mSettings(settings)
{
    mStreamingEvents = mEwsClient.serverVersion().supports(EwsServerVersion::StreamingSubscription);
    mStreamingTimer.setInterval(streamingConnTimeout * 1000);
    mStreamingTimer.setSingleShot(true);
    connect(&mStreamingTimer, &QTimer::timeout, this, &EwsSubscriptionManager::streamingConnectionTimeout);
}

EwsSubscriptionManager::~EwsSubscriptionManager()
{
    cancelSubscription();
}

void EwsSubscriptionManager::start()
{
    // Set-up change notification subscription (if needed)
    if (mSettings->eventSubscriptionId().isEmpty()) {
        setupSubscription();
    } else {
        reset();
    }

    if (!mStreamingEvents) {
        mPollTimer.setInterval(pollInterval * 1000);
        mPollTimer.setSingleShot(false);
        connect(&mPollTimer, &QTimer::timeout, this, &EwsSubscriptionManager::getEvents);
    }
}

void EwsSubscriptionManager::cancelSubscription()
{
    if (!mSettings->eventSubscriptionId().isEmpty()) {
        QPointer<EwsUnsubscribeRequest> req = new EwsUnsubscribeRequest(mEwsClient, this);
        req->setSubscriptionId(mSettings->eventSubscriptionId());
        req->exec();
        mSettings->setEventSubscriptionId(QString());
        mSettings->setEventSubscriptionWatermark(QString());
        mSettings->save();
    }
}

void EwsSubscriptionManager::setupSubscription()
{
    auto job = new EwsSubscribedFoldersJob(mEwsClient, mSettings, this);
    connect(job, &EwsRequest::result, this, &EwsSubscriptionManager::verifySubFoldersRequestFinished);
    job->start();
}

void EwsSubscriptionManager::verifySubFoldersRequestFinished(KJob *job)
{
    if (!job->error()) {
        auto folderJob = qobject_cast<EwsSubscribedFoldersJob *>(job);
        Q_ASSERT(folderJob);

        setupSubscriptionReq(folderJob->folders());
    } else {
        Q_EMIT connectionError();
    }
}

void EwsSubscriptionManager::setupSubscriptionReq(const EwsId::List &ids)
{
    auto req = new EwsSubscribeRequest(mEwsClient, this);
    // req->setAllFolders(true);
    QList<EwsEventType> events;
    events << EwsNewMailEvent;
    events << EwsMovedEvent;
    events << EwsCopiedEvent;
    events << EwsModifiedEvent;
    events << EwsDeletedEvent;
    events << EwsCreatedEvent;
    req->setEventTypes(events);
    if (mStreamingEvents) {
        req->setType(EwsSubscribeRequest::StreamingSubscription);
    } else {
        req->setType(EwsSubscribeRequest::PullSubscription);
    }
    req->setFolderIds(ids);
    req->setAllFolders(false);
    connect(req, &EwsRequest::result, this, &EwsSubscriptionManager::subscribeRequestFinished);
    req->start();
}

void EwsSubscriptionManager::reset()
{
    mPollTimer.stop();
    getEvents();
    if (!mStreamingEvents) {
        mPollTimer.start();
    }
}

void EwsSubscriptionManager::resetSubscription()
{
    mPollTimer.stop();
    cancelSubscription();
    setupSubscription();
}

void EwsSubscriptionManager::subscribeRequestFinished(KJob *job)
{
    if (!job->error()) {
        auto req = qobject_cast<EwsSubscribeRequest *>(job);
        if (req) {
            mSettings->setEventSubscriptionId(req->response().subscriptionId());
            if (mStreamingEvents) {
                getEvents();
            } else {
                mSettings->setEventSubscriptionWatermark(req->response().watermark());
                getEvents();
                mPollTimer.start();
            }
            mSettings->save();
        }
    } else {
        Q_EMIT connectionError();
    }
}

void EwsSubscriptionManager::getEvents()
{
    if (mStreamingEvents) {
        auto req = new EwsGetStreamingEventsRequest(mEwsClient, this);
        req->setSubscriptionId(mSettings->eventSubscriptionId());
        req->setTimeout(streamingTimeout);
        connect(req, &EwsRequest::result, this, &EwsSubscriptionManager::getEventsRequestFinished);
        connect(req, &EwsGetStreamingEventsRequest::eventsReceived, this, &EwsSubscriptionManager::streamingEventsReceived);
        req->start();
        mEventReq = req;
        mStreamingTimer.start();
    } else {
        auto req = new EwsGetEventsRequest(mEwsClient, this);
        req->setSubscriptionId(mSettings->eventSubscriptionId());
        req->setWatermark(mSettings->eventSubscriptionWatermark());
        connect(req, &EwsRequest::result, this, &EwsSubscriptionManager::getEventsRequestFinished);
        req->start();
        mEventReq = req;
    }
}

void EwsSubscriptionManager::getEventsRequestFinished(KJob *job)
{
    mStreamingTimer.stop();

    mEventReq->deleteLater();
    mEventReq = nullptr;

    auto req = qobject_cast<EwsEventRequestBase *>(job);
    if (!req) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Invalid EwsEventRequestBase job object.");
        reset();
        return;
    }

    if ((!req->responses().isEmpty())
        && ((req->responses()[0].responseCode() == QLatin1String("ErrorInvalidSubscription"))
            || (req->responses()[0].responseCode() == QLatin1String("ErrorSubscriptionNotFound")))) {
        mSettings->setEventSubscriptionId(QString());
        mSettings->setEventSubscriptionWatermark(QString());
        mSettings->save();
        resetSubscription();
        return;
    }

    if (!job->error()) {
        processEvents(req, true);
        if (mStreamingEvents) {
            getEvents();
        }
    } else {
        reset();
    }
}

void EwsSubscriptionManager::streamingEventsReceived(KJob *job)
{
    mStreamingTimer.stop();

    auto req = qobject_cast<EwsEventRequestBase *>(job);
    if (!req) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Invalid EwsEventRequestBase job object.");
        reset();
        return;
    }

    if (!job->error()) {
        processEvents(req, false);
        mStreamingTimer.start();
    }
}

void EwsSubscriptionManager::streamingConnectionTimeout()
{
    if (mEventReq) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Streaming request timeout - restarting");
        mEventReq->deleteLater();
        mEventReq = nullptr;
        getEvents();
    }
}

void EwsSubscriptionManager::processEvents(EwsEventRequestBase *req, bool finished)
{
    bool moreEvents = false;

    Q_FOREACH (const EwsGetEventsRequest::Response &resp, req->responses()) {
        Q_FOREACH (const EwsGetEventsRequest::Notification &nfy, resp.notifications()) {
            Q_FOREACH (const EwsGetEventsRequest::Event &event, nfy.events()) {
                bool skip = false;
                EwsId id = event.itemId();
                for (auto it = mQueuedUpdates.find(id.id()); it != mQueuedUpdates.end(); ++it) {
                    if (it->type == event.type() && (it->type == EwsDeletedEvent || it->changeKey == id.changeKey())) {
                        qCDebugNC(EWSRES_LOG) << QStringLiteral("Skipped queued update type %1 for item %2");
                        skip = true;
                        mQueuedUpdates.erase(it);
                        break;
                    }
                }

                mSettings->setEventSubscriptionWatermark(event.watermark());
                if (!skip) {
                    switch (event.type()) {
                    case EwsCopiedEvent:
                    case EwsMovedEvent:
                        if (!event.itemIsFolder()) {
                            mUpdatedFolderIds.insert(event.oldParentFolderId());
                        }
                    /* fall through */
                    case EwsCreatedEvent:
                    case EwsDeletedEvent:
                    case EwsModifiedEvent:
                    case EwsNewMailEvent:
                        if (event.itemIsFolder()) {
                            mFolderTreeChanged = true;
                        } else {
                            mUpdatedFolderIds.insert(event.parentFolderId());
                        }
                        break;
                    case EwsStatusEvent:
                        // Do nothing
                        break;
                    default:
                        break;
                    }
                }
            }
            if (nfy.hasMoreEvents()) {
                moreEvents = true;
            }
        }
        if (mStreamingEvents) {
            auto req2 = qobject_cast<EwsGetStreamingEventsRequest *>(req);
            if (req2) {
                req2->eventsProcessed(resp);
            }
        }
    }

    if (moreEvents && finished) {
        getEvents();
    } else {
        if (mFolderTreeChanged) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Found modified folder tree");
            Q_EMIT folderTreeModified();
            mFolderTreeChanged = false;
        }
        if (!mUpdatedFolderIds.isEmpty()) {
            qCDebugNC(EWSRES_LOG) << QStringLiteral("Found %1 modified folders").arg(mUpdatedFolderIds.size());
            Q_EMIT foldersModified(mUpdatedFolderIds.values());
            mUpdatedFolderIds.clear();
        }
    }
}

void EwsSubscriptionManager::queueUpdate(EwsEventType type, const QString &id, const QString &changeKey)
{
    mQueuedUpdates.insert(id, {type, changeKey});
}
