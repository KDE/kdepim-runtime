/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "sendjob.h"

#include "storeresultjob.h"

#include <agentinstance.h>
#include <agentmanager.h>
#include <collection.h>
#include <kdbusconnectionpool.h>
#include <item.h>
#include <itemdeletejob.h>
#include <itemmodifyjob.h>
#include <itemmovejob.h>
#include <collectionfetchjob.h>
#include <Akonadi/KMime/AddressAttribute>
#include <Akonadi/KMime/MessageParts>
#include <Akonadi/KMime/SpecialMailCollections>
#include <transportresourcebase.h>
#include "maildispatcher_debug.h"
#include <klocalizedstring.h>
#include <kmime/kmime_message.h>
#include <mailtransportakonadi/sentbehaviourattribute.h>
#include <mailtransport/transport.h>
#include <mailtransportakonadi/transportattribute.h>
#include <mailtransport/transportjob.h>
#include <mailtransport/transportmanager.h>
#include <AkonadiCore/ServerManager>

#include <QTimer>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

using namespace Akonadi;
using namespace KMime;
using namespace MailTransport;

/**
 * Private class that helps to provide binary compatibility between releases.
 * @internal
 */
class SendJob::Private
{
public:
    Private(const Item &itm, SendJob *qq)
        : q(qq),
          item(itm),
          currentJob(nullptr),
          interface(nullptr),
          aborting(false)
    {
    }

    SendJob *const q;
    Item item;
    KJob *currentJob = nullptr;
    QString resourceId;
    QDBusInterface *interface = nullptr;
    bool aborting = false;

    void doAkonadiTransport();
    void doTraditionalTransport();
    void doPostJob(bool transportSuccess, const QString &transportMessage);
    void storeResult(bool success, const QString &message = QString());
    void abortPostJob();
    bool filterItem(int filterset);

    // slots
    void doTransport();
    void transportPercent(KJob *job, unsigned long percent);
    void transportResult(KJob *job);
    void resourceProgress(const AgentInstance &instance);
    void resourceResult(qlonglong itemId, int result, const QString &message);
    void postJobResult(KJob *job);
    void doEmitResult(KJob *job);
    void slotSentMailCollectionFetched(KJob *job);
};

void SendJob::Private::doTransport()
{
    qCDebug(MAILDISPATCHER_LOG) << "Transporting message.";

    if (aborting) {
        qCDebug(MAILDISPATCHER_LOG) << "Marking message as aborted.";
        q->setError(UserDefinedError);
        q->setErrorText(i18n("Message sending aborted."));
        storeResult(false, i18n("Message sending aborted."));
        return;
    }

    // Is it an Akonadi transport or a traditional one?
    const TransportAttribute *transportAttribute = item.attribute<TransportAttribute>();
    Q_ASSERT(transportAttribute);
    if (!transportAttribute->transport()) {
        storeResult(false, i18n("Could not initiate message transport. Possibly invalid transport."));
        return;
    }

    const TransportType type = transportAttribute->transport()->transportType();
    if (!type.isValid()) {
        storeResult(false, i18n("Could not send message. Invalid transport."));
        return;
    }

    if (!filterItem(8)) {   //BeforeOutbound
        return;
    }

    if (type.isAkonadiResource()) {
        // Send the item directly to the resource that will send it.
        resourceId = transportAttribute->transport()->host();
        doAkonadiTransport();
    } else {
        // Use a traditional transport job.
        doTraditionalTransport();
    }
}

void SendJob::Private::doAkonadiTransport()
{
    Q_ASSERT(!resourceId.isEmpty());
    Q_ASSERT(interface == nullptr);

    const auto service = Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Resource,
                                                                  resourceId);

    interface = new QDBusInterface(service,
                                   QStringLiteral("/Transport"),
                                   QStringLiteral("org.freedesktop.Akonadi.Resource.Transport"),
                                   KDBusConnectionPool::threadConnection(), q);

    if (!interface->isValid()) {
        storeResult(false, i18n("Failed to get D-Bus interface of resource %1.", resourceId));
        delete interface;
        interface = nullptr;
        return;
    }

    // Signals.
    QObject::connect(AgentManager::self(), SIGNAL(instanceProgressChanged(Akonadi::AgentInstance)),
                     q, SLOT(resourceProgress(Akonadi::AgentInstance)));
    QObject::connect(interface, SIGNAL(transportResult(qlonglong,int,QString)),
                     q, SLOT(resourceResult(qlonglong,int,QString)));

    // Start sending.
    const QDBusReply<void> reply = interface->call(QStringLiteral("send"), item.id());
    if (!reply.isValid()) {
        storeResult(false, i18n("Invalid D-Bus reply from resource %1.", resourceId));
        return;
    }
}

void SendJob::Private::doTraditionalTransport()
{
    const TransportAttribute *transportAttribute = item.attribute<TransportAttribute>();
    TransportJob *job = TransportManager::self()->createTransportJob(transportAttribute->transportId());

    Q_ASSERT(job);
    Q_ASSERT(currentJob == nullptr);

    currentJob = job;

    // Message.
    Q_ASSERT(item.hasPayload<Message::Ptr>());
    const Message::Ptr message = item.payload<Message::Ptr>();
    bool needAssemble = false;
    if (message->removeHeader("Bcc")) {
        needAssemble = true;
    }
    if (message->removeHeader("X-KMail-Identity")) {
        needAssemble = true;
    }
    if (message->removeHeader("X-KMail-Dictionary")) {
        needAssemble = true;
    }

    if (needAssemble) {
        message->assemble();
    }
    const QByteArray content = message->encodedContent(true) + "\r\n";
    Q_ASSERT(!content.isEmpty());

    // Addresses.
    const AddressAttribute *addressAttribute = item.attribute<AddressAttribute>();
    Q_ASSERT(addressAttribute);

    job->setData(content);
    job->setSender(addressAttribute->from());
    job->setTo(addressAttribute->to());
    job->setCc(addressAttribute->cc());
    job->setBcc(addressAttribute->bcc());

    // Signals.
    connect(job, &TransportJob::result, q, [this](KJob *job) { transportResult(job);});
    //We have a private signal
    //connect(job, QOverload<KJob*, ulong>::of(&TransportJob::percent), q, [this](KJob *job,ulong val) {transportPercent(job, val); });
    connect(job, SIGNAL(percent(KJob*,ulong)),
                q, SLOT(transportPercent(KJob*,ulong)));
    job->start();
}

void SendJob::Private::transportPercent(KJob *job, unsigned long)
{
    Q_ASSERT(currentJob == job);
    qCDebug(MAILDISPATCHER_LOG) << "Processed amount" << job->processedAmount(KJob::Bytes)
                                << "total amount" << job->totalAmount(KJob::Bytes);

    q->setTotalAmount(KJob::Bytes, job->totalAmount(KJob::Bytes));     // Is not set at the time of start().
    q->setProcessedAmount(KJob::Bytes, job->processedAmount(KJob::Bytes));
}

void SendJob::Private::transportResult(KJob *job)
{
    Q_ASSERT(currentJob == job);
    currentJob = nullptr;
    doPostJob(!job->error(), job->errorString());
}

void SendJob::Private::resourceProgress(const AgentInstance &instance)
{
    if (!interface) {
        // We might have gotten a very late signal.
        qCWarning(MAILDISPATCHER_LOG) << "called but no resource job running!";
        return;
    }

    if (instance.identifier() == resourceId) {
        // This relies on the resource's progress representing the progress of
        // sending this item.
        q->setPercent(instance.progress());
    }
}

void SendJob::Private::resourceResult(qlonglong itemId, int result,
                                      const QString &message)
{
    Q_UNUSED(itemId);
    Q_ASSERT(interface);
    delete interface; // So that abort() knows the transport job is over.
    interface = nullptr;

    const TransportResourceBase::TransportResult transportResult =
        static_cast<TransportResourceBase::TransportResult>(result);

    const bool success = (transportResult == TransportResourceBase::TransportSucceeded);

    Q_ASSERT(itemId == item.id());
    doPostJob(success, message);
}

void SendJob::Private::abortPostJob()
{
    // We were unlucky and LocalFolders is recreating its stuff right now.
    // We will not wait for it.
    qCWarning(MAILDISPATCHER_LOG) << "Default sent mail collection unavailable, not moving the mail after sending.";
    q->setError(UserDefinedError);
    q->setErrorText(i18n("Default sent-mail folder unavailable. Keeping message in outbox."));
    storeResult(false, q->errorString());
}

void SendJob::Private::doPostJob(bool transportSuccess, const QString &transportMessage)
{
    qCDebug(MAILDISPATCHER_LOG) << "success" << transportSuccess << "message" << transportMessage;

    if (!transportSuccess) {
        qCDebug(MAILDISPATCHER_LOG) << "Error transporting.";
        q->setError(UserDefinedError);

        const QString error = aborting ? i18n("Message transport aborted.")
                              : i18n("Failed to transport message.");

        q->setErrorText(error + QLatin1Char(' ') + transportMessage);
        storeResult(false, q->errorString());
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Success transporting.";

        // Delete or move to sent-mail.
        const SentBehaviourAttribute *attribute = item.attribute<SentBehaviourAttribute>();
        Q_ASSERT(attribute);

        if (attribute->sentBehaviour() == SentBehaviourAttribute::Delete) {
            qCDebug(MAILDISPATCHER_LOG) << "Deleting item from outbox.";
            currentJob = new ItemDeleteJob(item);
            QObject::connect(currentJob, &ItemDeleteJob::result, q, [this](KJob *job) { postJobResult(job); });
        } else {
            if (attribute->sentBehaviour() == SentBehaviourAttribute::MoveToDefaultSentCollection) {
                if (SpecialMailCollections::self()->hasDefaultCollection(SpecialMailCollections::SentMail)) {
                    currentJob = new ItemMoveJob(item, SpecialMailCollections::self()->defaultCollection(SpecialMailCollections::SentMail), q);
                    QObject::connect(currentJob, &ItemMoveJob::result, q, [this](KJob *job) { postJobResult(job); });
                } else {
                    abortPostJob();
                }
            } else {
                qCDebug(MAILDISPATCHER_LOG) << "sentBehaviour=" << attribute->sentBehaviour() << "using collection from attribute";
                currentJob = new CollectionFetchJob(attribute->moveToCollection(), Akonadi::CollectionFetchJob::Base);
                QObject::connect(currentJob, &CollectionFetchJob::result, q, [this](KJob *job) { slotSentMailCollectionFetched(job);});

            }
        }
    }
}

bool SendJob::Private::filterItem(int filterset)
{
    const auto service = Akonadi::ServerManager::agentServiceName(Akonadi::ServerManager::Agent,
                                                                  QStringLiteral("akonadi_mailfilter_agent"));

    QDBusInterface iface(service,
                         QStringLiteral("/MailFilterAgent"),
                         QStringLiteral("org.freedesktop.Akonadi.MailFilterAgent"),
                         KDBusConnectionPool::threadConnection(), q);
    if (!iface.isValid()) {
        storeResult(false, i18n("Failed to get D-Bus interface of mailfilteragent."));
        return false;
    }

    //Outbound = 0x2
    const QDBusReply<void> reply = iface.call(QStringLiteral("filterItem"), item.id(), filterset, QString());
    if (!reply.isValid()) {
        storeResult(false, i18n("Invalid D-Bus reply from mailfilteragent"));
        return false;
    }

    return true;
}

void SendJob::Private::slotSentMailCollectionFetched(KJob *job)
{
    Akonadi::Collection fetchCol;
    bool ok = false;
    if (!job->error()) {
        const CollectionFetchJob *const fetchJob = qobject_cast<CollectionFetchJob *>(job);
        if (!fetchJob->collections().isEmpty()) {
            fetchCol = fetchJob->collections().at(0);
            ok = true;
        }
    }
    if (!ok) {
        if (!SpecialMailCollections::self()->hasDefaultCollection(SpecialMailCollections::SentMail)) {
            abortPostJob();
            return;
        }
        fetchCol = SpecialMailCollections::self()->defaultCollection(SpecialMailCollections::SentMail);
    }
    currentJob = new ItemMoveJob(item, fetchCol, q);
    QObject::connect(currentJob, &ItemMoveJob::result, q, [this](KJob *job) {postJobResult(job);});
}

void SendJob::Private::postJobResult(KJob *job)
{
    Q_ASSERT(currentJob == job);
    currentJob = nullptr;
    const SentBehaviourAttribute *attribute = item.attribute<SentBehaviourAttribute>();
    Q_ASSERT(attribute);

    if (job->error()) {
        qCDebug(MAILDISPATCHER_LOG) << "Error deleting or moving to sent-mail.";

        QString errorString;
        switch (attribute->sentBehaviour()) {
        case SentBehaviourAttribute::Delete:
            errorString =
                i18n("Sending succeeded, but failed to remove the message from the outbox.");
            break;
        default:
            errorString =
                i18n("Sending succeeded, but failed to move the message to the sent-mail folder.");
            break;
        }
        q->setError(UserDefinedError);
        q->setErrorText(errorString + QLatin1Char(' ') + job->errorString());
        storeResult(false, q->errorString());
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Success deleting or moving to sent-mail.";
        if (!filterItem(2)) {   //Outbound
            return;
        }
        if (attribute->sentBehaviour() == SentBehaviourAttribute::Delete) {
            q->emitResult();
        } else {
            storeResult(true);
        }
    }
}

void SendJob::Private::storeResult(bool success, const QString &message)
{
    qCDebug(MAILDISPATCHER_LOG) << "success" << success << "message" << message;

    Q_ASSERT(currentJob == nullptr);
    currentJob = new StoreResultJob(item, success, message);
    connect(currentJob, &StoreResultJob::result, [this](KJob *job) { doEmitResult(job); });
}

void SendJob::Private::doEmitResult(KJob *job)
{
    Q_ASSERT(currentJob == job);
    currentJob = nullptr;

    if (job->error()) {
        qCWarning(MAILDISPATCHER_LOG) << "Error storing result.";
        q->setError(UserDefinedError);
        q->setErrorText(q->errorString() + QLatin1Char(' ') + i18n("Failed to store result in item.") + QLatin1Char(' ') + job->errorString());
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Success storing result.";
        // It is still possible that the transport failed.
        StoreResultJob *srJob = static_cast<StoreResultJob *>(job);
        if (!srJob->success()) {
            q->setError(UserDefinedError);
            q->setErrorText(srJob->message());
        }
    }

    q->emitResult();
}

SendJob::SendJob(const Item &item, QObject *parent)
    : KJob(parent),
      d(new Private(item, this))
{
}

SendJob::~SendJob()
{
    delete d;
}

void SendJob::start()
{
    QTimer::singleShot(0, this, SLOT(doTransport()));
}

void SendJob::setMarkAborted()
{
    Q_ASSERT(!d->aborting);
    d->aborting = true;
}

void SendJob::abort()
{
    setMarkAborted();

    if (dynamic_cast<TransportJob *>(d->currentJob)) {
        qCDebug(MAILDISPATCHER_LOG) << "Abort called, active transport job.";
        // Abort transport.
        d->currentJob->kill(KJob::EmitResult);
    } else if (d->interface != nullptr) {
        qCDebug(MAILDISPATCHER_LOG) << "Abort called, propagating to resource.";
        // Abort resource doing transport.
        AgentInstance instance = AgentManager::self()->instance(d->resourceId);
        instance.abortCurrentTask();
    } else {
        qCDebug(MAILDISPATCHER_LOG) << "Abort called, but no transport job is active.";
        // Either transport has not started, in which case doTransport will
        // mark the item as aborted, or the item has already been sent, in which
        // case there is nothing we can do.
    }
}

#include "moc_sendjob.cpp"
