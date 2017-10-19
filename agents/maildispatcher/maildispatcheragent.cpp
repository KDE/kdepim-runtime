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

#include <kdbusconnectionpool.h>
#include <itemfetchscope.h>
#include <mailtransportakonadi/sentactionattribute.h>
#include <mailtransportakonadi/sentbehaviourattribute.h>
#include <AkonadiCore/ServerManager>

#include <knotifyconfigwidget.h>
#include "maildispatcher_debug.h"
#include <KLocalizedString>
#include <KMime/Message>
#include <KNotification>
#include <Kdelibs4ConfigMigrator>

#include <QTimer>
#include <QDBusConnection>

#ifdef MAIL_SERIALIZER_PLUGIN_STATIC

Q_IMPORT_PLUGIN(akonadi_serializer_mail)
#endif

using namespace Akonadi;


void MailDispatcherAgent::abort()
{
    if (!isOnline()) {
        qCDebug(MAILDISPATCHER_LOG) << "Offline. Ignoring call.";
        return;
    }

    if (mAborting) {
        qCDebug(MAILDISPATCHER_LOG) << "Already aborting.";
        return;
    }

    if (!mSendingInProgress && mQueue->isEmpty()) {
        qCDebug(MAILDISPATCHER_LOG) << "MDA is idle.";
        Q_ASSERT(status() == AgentBase::Idle);
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Aborting...";
        mAborting = true;
        if (mCurrentJob) {
            mCurrentJob->abort();
        }
        // Further SendJobs will mark remaining items in the queue as 'aborted'.
    }
}

void MailDispatcherAgent::dispatch()
{
    Q_ASSERT(mQueue);

    if (!isOnline() || mSendingInProgress) {
        qCDebug(MAILDISPATCHER_LOG) << "Offline or busy. See you later.";
        return;
    }

    if (!mQueue->isEmpty()) {
        if (!mSentAnything) {
            mSentAnything = true;
            mSentItemsSize = 0;
            Q_EMIT percent(0);
        }
        Q_EMIT status(AgentBase::Running,
                         i18np("Sending messages (1 item in queue)...",
                               "Sending messages (%1 items in queue)...", mQueue->count()));
        qCDebug(MAILDISPATCHER_LOG) << "Attempting to dispatch the next message.";
        mSendingInProgress = true;
        mQueue->fetchOne(); // will trigger itemFetched
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Empty queue.";
        if (mAborting) {
            // Finished marking messages as 'aborted'.
            mAborting = false;
            mSentAnything = false;
            Q_EMIT status(AgentBase::Idle, i18n("Sending canceled."));
            QTimer::singleShot(3000, this, [this]() {emitStatusReady();});
        } else {
            if (mSentAnything) {
                // Finished sending messages in queue.
                mSentAnything = false;
                Q_EMIT percent(100);
                Q_EMIT status(AgentBase::Idle, i18n("Finished sending messages."));

                if (!mErrorOccurred && mShowSentNotification) {
                    KNotification *notify = new KNotification(QStringLiteral("emailsent"));
                    notify->setComponentName(QStringLiteral("akonadi_maildispatcher_agent"));
                    notify->setTitle(i18nc("Notification title when email was sent", "E-Mail Successfully Sent"));
                    notify->setText(i18nc("Notification when the email was sent", "Your E-Mail has been sent successfully."));
                    notify->sendEvent();
                }
                mShowSentNotification = true;
            } else {
                // Empty queue.
                Q_EMIT status(AgentBase::Idle, i18n("No items in queue."));
            }
            QTimer::singleShot(3000, this, [this]() {emitStatusReady();});
        }

        mErrorOccurred = false;
    }
}

MailDispatcherAgent::MailDispatcherAgent(const QString &id)
    : AgentBase(id)
{
    Kdelibs4ConfigMigrator migrate(QStringLiteral("maildispatcheragent"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("maildispatcheragentrc") << QStringLiteral("akonadi_maildispatcher_agent.notifyrc"));
    migrate.migrate();

    qCDebug(MAILDISPATCHER_LOG) << "maildispatcheragent: At your service, sir!";

    new SettingsAdaptor(Settings::self());
    new MailDispatcherAgentAdaptor(this);

    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/Settings"),
            Settings::self(), QDBusConnection::ExportAdaptors);

    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/MailDispatcherAgent"),
            this, QDBusConnection::ExportAdaptors);
    QString service = QStringLiteral("org.freedesktop.Akonadi.MailDispatcherAgent");
    if (Akonadi::ServerManager::hasInstanceIdentifier()) {
        service += QLatin1Char('.') + Akonadi::ServerManager::instanceIdentifier();
    }

    KDBusConnectionPool::threadConnection().registerService(service);

    mQueue = new OutboxQueue(this);
    connect(mQueue, &OutboxQueue::newItems,
            this, [this]() { dispatch(); });
    connect(mQueue, &OutboxQueue::itemReady,
            this, [this](const Akonadi::Item &item) { itemFetched(item);});
    connect(mQueue, &OutboxQueue::error,
            this, &MailDispatcherAgent::queueError);
    connect(this, &MailDispatcherAgent::itemProcessed,
            mQueue, &OutboxQueue::itemProcessed);
    connect(this, &MailDispatcherAgent::abortRequested,
            this, [this]() { abort(); });

    mSentActionHandler = new SentActionHandler(this);

    setNeedsNetwork(true);
}

MailDispatcherAgent::~MailDispatcherAgent()
{
}

void MailDispatcherAgent::configure(WId windowId)
{
    Q_UNUSED(windowId);
    KNotifyConfigWidget::configure(nullptr);
}

void MailDispatcherAgent::doSetOnline(bool online)
{
    Q_ASSERT(mQueue);
    if (online) {
        qCDebug(MAILDISPATCHER_LOG) << "Online. Dispatching messages.";
        Q_EMIT status(AgentBase::Idle, i18n("Online, sending messages in queue."));
        QTimer::singleShot(0, this, [this]() { dispatch(); });
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Offline.";
        Q_EMIT status(AgentBase::Idle, i18n("Offline, message sending suspended."));

        // TODO: This way, the OutboxQueue will continue to react to changes in
        // the outbox, but the MDA will just not send anything.  Is this what we
        // want?
    }

    AgentBase::doSetOnline(online);
}

void MailDispatcherAgent::itemFetched(const Item &item)
{
    qCDebug(MAILDISPATCHER_LOG) << "Fetched item" << item.id() << "; creating SendJob.";
    Q_ASSERT(mSendingInProgress);
    Q_ASSERT(!mCurrentItem.isValid());
    mCurrentItem = item;
    Q_ASSERT(mCurrentJob == nullptr);
    Q_EMIT itemDispatchStarted();

    mCurrentJob = new SendJob(item, this);
    if (mAborting) {
        mCurrentJob->setMarkAborted();
    }

    Q_EMIT status(AgentBase::Running, i18nc("Message with given subject is being sent.", "Sending: %1",
                                        item.payload<KMime::Message::Ptr>()->subject()->asUnicodeString()));

    connect(mCurrentJob, &KJob::result,
            this, &MailDispatcherAgent::sendResult);
    connect(mCurrentJob, SIGNAL(percent(KJob*,ulong)),
            this, SLOT(sendPercent(KJob*,ulong)));

    mCurrentJob->start();
}

void MailDispatcherAgent::queueError(const QString &message)
{
    Q_EMIT error(message);
    mErrorOccurred = true;
    // FIXME figure out why this does not set the status to Broken, etc.
}

void MailDispatcherAgent::sendPercent(KJob *job, unsigned long)
{
    Q_ASSERT(mSendingInProgress);
    Q_ASSERT(job == mCurrentJob);
    // The progress here is actually the TransportJob, not the entire SendJob,
    // because the post-job doesn't report progress.  This should be fine,
    // since the TransportJob is the lengthiest operation.

    // Give the transport 80% of the weight, and move-to-sendmail 20%.
    const double transportWeight = 0.8;

    const int percentValue = 100 * (mSentItemsSize + job->processedAmount(KJob::Bytes) * transportWeight)
                        / (mSentItemsSize + mCurrentItem.size() + mQueue->totalSize());

    qCDebug(MAILDISPATCHER_LOG) << "sentItemsSize" << mSentItemsSize
                                << "this job processed" << job->processedAmount(KJob::Bytes)
                                << "queue totalSize" << mQueue->totalSize()
                                << "total total size (sent+current+queue)" << (mSentItemsSize + mCurrentItem.size() + mQueue->totalSize())
                                << "new percentage" << percentValue << "old percentage" << progress();

    if (percentValue != progress()) {
        // The progress can decrease too, if messages got added to the queue.
        Q_EMIT percent(percentValue);
    }

    // It is possible that the number of queued messages has changed.
    Q_EMIT status(AgentBase::Running,
                     i18np("Sending messages (1 item in queue)...",
                           "Sending messages (%1 items in queue)...", 1 + mQueue->count()));
}

void MailDispatcherAgent::sendResult(KJob *job)
{
    Q_ASSERT(mSendingInProgress);
    Q_ASSERT(job == mCurrentJob);
    mCurrentJob->disconnect(this);
    mCurrentJob = nullptr;

    Q_ASSERT(mCurrentItem.isValid());
    mSentItemsSize += mCurrentItem.size();
    Q_EMIT itemProcessed(mCurrentItem, !job->error());

    const Akonadi::Item sentItem = mCurrentItem;
    mCurrentItem = Item();

    if (job->error()) {
        // The SendJob gave the item an ErrorAttribute, so we don't have to
        // do anything.
        qCDebug(MAILDISPATCHER_LOG) << "Sending failed. error:" << job->errorString();

        KNotification *notify = new KNotification(QStringLiteral("sendingfailed"));
        notify->setComponentName(QStringLiteral("akonadi_maildispatcher_agent"));
        notify->setTitle(i18nc("Notification title when email sending failed", "E-Mail Sending Failed"));
        notify->setText(job->errorString());
        notify->sendEvent();

        mErrorOccurred = true;
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Sending succeeded.";

        // handle possible sent actions
        const MailTransport::SentActionAttribute *attribute = sentItem.attribute<MailTransport::SentActionAttribute>();
        if (attribute) {
            const MailTransport::SentActionAttribute::Action::List lstAct = attribute->actions();
            for (const MailTransport::SentActionAttribute::Action &action : lstAct) {
                mSentActionHandler->runAction(action);
            }
        }
        const auto bhAttribute = sentItem.attribute<MailTransport::SentBehaviourAttribute>();
        if (bhAttribute) {
            mShowSentNotification = !bhAttribute->sendSilently();
        } else {
            mShowSentNotification = true;
        }
    }

    // dispatch next message
    mSendingInProgress = false;
    QTimer::singleShot(0, this, [this]() { dispatch();});
}

void MailDispatcherAgent::emitStatusReady()
{
    if (status() == AgentBase::Idle) {
        // If still idle after aborting, clear 'aborted' status.
        Q_EMIT status(AgentBase::Idle, i18n("Ready to dispatch messages."));
    }
}

#ifndef KDEPIM_PLUGIN_AGENT
AKONADI_AGENT_MAIN(MailDispatcherAgent)
#endif

#include "moc_maildispatcheragent.cpp"
