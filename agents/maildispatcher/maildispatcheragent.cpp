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
#include <mailtransport/sentactionattribute.h>
#include <mailtransport/sentbehaviourattribute.h>

#include <knotifyconfigwidget.h>
#include "maildispatcher_debug.h"
#include <KLocalizedString>
#include <KMime/Message>
#include <KNotification>
#include <Kdelibs4ConfigMigrator>

#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>

#ifdef MAIL_SERIALIZER_PLUGIN_STATIC

Q_IMPORT_PLUGIN(akonadi_serializer_mail)
#endif

using namespace Akonadi;

class MailDispatcherAgent::Private
{
public:
    Private(MailDispatcherAgent *parent)
        : q(parent),
          queue(nullptr),
          currentJob(nullptr),
          aborting(false),
          sendingInProgress(false),
          sentAnything(false),
          errorOccurred(false),
          showSentNotification(true),
          sentItemsSize(0),
          sentActionHandler(nullptr)
    {
    }

    ~Private()
    {
    }

    MailDispatcherAgent *const q;

    OutboxQueue *queue;
    SendJob *currentJob;
    Item currentItem;
    bool aborting;
    bool sendingInProgress;
    bool sentAnything;
    bool errorOccurred;
    bool showSentNotification;
    qulonglong sentItemsSize;
    SentActionHandler *sentActionHandler;

    // Q_SLOTS:
    void abort();
    void dispatch();
    void itemFetched(const Item &item);
    void queueError(const QString &message);
    void sendPercent(KJob *job, unsigned long percent);
    void sendResult(KJob *job);
    void emitStatusReady();
};

void MailDispatcherAgent::Private::abort()
{
    if (!q->isOnline()) {
        qCDebug(MAILDISPATCHER_LOG) << "Offline. Ignoring call.";
        return;
    }

    if (aborting) {
        qCDebug(MAILDISPATCHER_LOG) << "Already aborting.";
        return;
    }

    if (!sendingInProgress && queue->isEmpty()) {
        qCDebug(MAILDISPATCHER_LOG) << "MDA is idle.";
        Q_ASSERT(q->status() == AgentBase::Idle);
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Aborting...";
        aborting = true;
        if (currentJob) {
            currentJob->abort();
        }
        // Further SendJobs will mark remaining items in the queue as 'aborted'.
    }
}

void MailDispatcherAgent::Private::dispatch()
{
    Q_ASSERT(queue);

    if (!q->isOnline() || sendingInProgress) {
        qCDebug(MAILDISPATCHER_LOG) << "Offline or busy. See you later.";
        return;
    }

    if (!queue->isEmpty()) {
        if (!sentAnything) {
            sentAnything = true;
            sentItemsSize = 0;
            Q_EMIT q->percent(0);
        }
        Q_EMIT q->status(AgentBase::Running,
                         i18np("Sending messages (1 item in queue)...",
                               "Sending messages (%1 items in queue)...", queue->count()));
        qCDebug(MAILDISPATCHER_LOG) << "Attempting to dispatch the next message.";
        sendingInProgress = true;
        queue->fetchOne(); // will trigger itemFetched
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Empty queue.";
        if (aborting) {
            // Finished marking messages as 'aborted'.
            aborting = false;
            sentAnything = false;
            Q_EMIT q->status(AgentBase::Idle, i18n("Sending canceled."));
            QTimer::singleShot(3000, q, SLOT(emitStatusReady()));
        } else {
            if (sentAnything) {
                // Finished sending messages in queue.
                sentAnything = false;
                Q_EMIT q->percent(100);
                Q_EMIT q->status(AgentBase::Idle, i18n("Finished sending messages."));

                if (!errorOccurred && showSentNotification) {
                    KNotification *notify = new KNotification(QStringLiteral("emailsent"));
                    notify->setComponentName(QStringLiteral("akonadi_maildispatcher_agent"));
                    notify->setTitle(i18nc("Notification title when email was sent", "E-Mail Successfully Sent"));
                    notify->setText(i18nc("Notification when the email was sent", "Your E-Mail has been sent successfully."));
                    notify->sendEvent();
                }
                showSentNotification = true;
            } else {
                // Empty queue.
                Q_EMIT q->status(AgentBase::Idle, i18n("No items in queue."));
            }
            QTimer::singleShot(3000, q, SLOT(emitStatusReady()));
        }

        errorOccurred = false;
    }
}

MailDispatcherAgent::MailDispatcherAgent(const QString &id)
    : AgentBase(id),
      d(new Private(this))
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
    KDBusConnectionPool::threadConnection().registerService(QStringLiteral("org.freedesktop.Akonadi.MailDispatcherAgent"));

    d->queue = new OutboxQueue(this);
    connect(d->queue, SIGNAL(newItems()),
            this, SLOT(dispatch()));
    connect(d->queue, SIGNAL(itemReady(Akonadi::Item)),
            this, SLOT(itemFetched(Akonadi::Item)));
    connect(d->queue, SIGNAL(error(QString)),
            this, SLOT(queueError(QString)));
    connect(this, SIGNAL(itemProcessed(Akonadi::Item,bool)),
            d->queue, SLOT(itemProcessed(Akonadi::Item,bool)));
    connect(this, SIGNAL(abortRequested()),
            this, SLOT(abort()));

    d->sentActionHandler = new SentActionHandler(this);

    setNeedsNetwork(true);
}

MailDispatcherAgent::~MailDispatcherAgent()
{
    delete d;
}

void MailDispatcherAgent::configure(WId windowId)
{
    Q_UNUSED(windowId);
    KNotifyConfigWidget::configure(nullptr);
}

void MailDispatcherAgent::doSetOnline(bool online)
{
    Q_ASSERT(d->queue);
    if (online) {
        qCDebug(MAILDISPATCHER_LOG) << "Online. Dispatching messages.";
        Q_EMIT status(AgentBase::Idle, i18n("Online, sending messages in queue."));
        QTimer::singleShot(0, this, SLOT(dispatch()));
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Offline.";
        Q_EMIT status(AgentBase::Idle, i18n("Offline, message sending suspended."));

        // TODO: This way, the OutboxQueue will continue to react to changes in
        // the outbox, but the MDA will just not send anything.  Is this what we
        // want?
    }

    AgentBase::doSetOnline(online);
}

void MailDispatcherAgent::Private::itemFetched(const Item &item)
{
    qCDebug(MAILDISPATCHER_LOG) << "Fetched item" << item.id() << "; creating SendJob.";
    Q_ASSERT(sendingInProgress);
    Q_ASSERT(!currentItem.isValid());
    currentItem = item;
    Q_ASSERT(currentJob == nullptr);
    Q_EMIT q->itemDispatchStarted();

    currentJob = new SendJob(item, q);
    if (aborting) {
        currentJob->setMarkAborted();
    }

    q->status(AgentBase::Running, i18nc("Message with given subject is being sent.", "Sending: %1",
                                        item.payload<KMime::Message::Ptr>()->subject()->asUnicodeString()));

    connect(currentJob, SIGNAL(result(KJob*)),
            q, SLOT(sendResult(KJob*)));
    connect(currentJob, SIGNAL(percent(KJob*,ulong)),
            q, SLOT(sendPercent(KJob*,ulong)));

    currentJob->start();
}

void MailDispatcherAgent::Private::queueError(const QString &message)
{
    Q_EMIT q->error(message);
    errorOccurred = true;
    // FIXME figure out why this does not set the status to Broken, etc.
}

void MailDispatcherAgent::Private::sendPercent(KJob *job, unsigned long)
{
    Q_ASSERT(sendingInProgress);
    Q_ASSERT(job == currentJob);
    // The progress here is actually the TransportJob, not the entire SendJob,
    // because the post-job doesn't report progress.  This should be fine,
    // since the TransportJob is the lengthiest operation.

    // Give the transport 80% of the weight, and move-to-sendmail 20%.
    const double transportWeight = 0.8;

    const int percent = 100 * (sentItemsSize + job->processedAmount(KJob::Bytes) * transportWeight)
                        / (sentItemsSize + currentItem.size() + queue->totalSize());

    qCDebug(MAILDISPATCHER_LOG) << "sentItemsSize" << sentItemsSize
                                << "this job processed" << job->processedAmount(KJob::Bytes)
                                << "queue totalSize" << queue->totalSize()
                                << "total total size (sent+current+queue)" << (sentItemsSize + currentItem.size() + queue->totalSize())
                                << "new percentage" << percent << "old percentage" << q->progress();

    if (percent != q->progress()) {
        // The progress can decrease too, if messages got added to the queue.
        Q_EMIT q->percent(percent);
    }

    // It is possible that the number of queued messages has changed.
    Q_EMIT q->status(AgentBase::Running,
                     i18np("Sending messages (1 item in queue)...",
                           "Sending messages (%1 items in queue)...", 1 + queue->count()));
}

void MailDispatcherAgent::Private::sendResult(KJob *job)
{
    Q_ASSERT(sendingInProgress);
    Q_ASSERT(job == currentJob);
    currentJob->disconnect(q);
    currentJob = nullptr;

    Q_ASSERT(currentItem.isValid());
    sentItemsSize += currentItem.size();
    Q_EMIT q->itemProcessed(currentItem, !job->error());

    const Akonadi::Item sentItem = currentItem;
    currentItem = Item();

    if (job->error()) {
        // The SendJob gave the item an ErrorAttribute, so we don't have to
        // do anything.
        qCDebug(MAILDISPATCHER_LOG) << "Sending failed. error:" << job->errorString();

        KNotification *notify = new KNotification(QStringLiteral("sendingfailed"));
        notify->setComponentName(QStringLiteral("akonadi_maildispatcher_agent"));
        notify->setTitle(i18nc("Notification title when email sending failed", "E-Mail Sending Failed"));
        notify->setText(job->errorString());
        notify->sendEvent();

        errorOccurred = true;
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Sending succeeded.";

        // handle possible sent actions
        const MailTransport::SentActionAttribute *attribute = sentItem.attribute<MailTransport::SentActionAttribute>();
        if (attribute) {
            foreach (const MailTransport::SentActionAttribute::Action &action, attribute->actions()) {
                sentActionHandler->runAction(action);
            }
        }
        const auto bhAttribute = sentItem.attribute<MailTransport::SentBehaviourAttribute>();
        if (bhAttribute) {
            showSentNotification = !bhAttribute->sendSilently();
        } else {
            showSentNotification = true;
        }
    }

    // dispatch next message
    sendingInProgress = false;
    QTimer::singleShot(0, q, SLOT(dispatch()));
}

void MailDispatcherAgent::Private::emitStatusReady()
{
    if (q->status() == AgentBase::Idle) {
        // If still idle after aborting, clear 'aborted' status.
        Q_EMIT q->status(AgentBase::Idle, i18n("Ready to dispatch messages."));
    }
}

#ifndef KDEPIM_PLUGIN_AGENT
AKONADI_AGENT_MAIN(MailDispatcherAgent)
#endif

#include "moc_maildispatcheragent.cpp"
