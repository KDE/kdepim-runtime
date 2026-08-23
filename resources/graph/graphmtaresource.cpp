/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphmtaresource.h"

#include "graph_debug.h"
#include "graphresourceinterface.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMime/Message>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

using namespace Akonadi;

static const auto kResourceServicePrefix = QLatin1String("org.freedesktop.Akonadi.Resource.akonadi_graph_resource");

GraphMtaResource::GraphMtaResource(const QString &id)
    : Akonadi::ResourceWidgetBase(id)
{
    setNeedsNetwork(true);
}

GraphMtaResource::~GraphMtaResource() = default;

bool GraphMtaResource::connectToMasterResource()
{
    if (mGraphResource) {
        return true;
    }
    // Which receive resource owns the account this transport sends through. Pinned in
    // the config; with a single Graph account the instance is adopted automatically.
    KConfigGroup group(config(), QStringLiteral("General"));
    const QString pinned = group.readEntry("MasterResource", QString());

    QStringList candidates;
    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
    for (const QString &service : services) {
        if (service.startsWith(kResourceServicePrefix)) {
            candidates.append(service);
        }
    }
    const QLatin1String serviceBase("org.freedesktop.Akonadi.Resource.");
    QString service;
    if (!pinned.isEmpty()) {
        service = serviceBase + pinned;
        if (!candidates.contains(service)) {
            qCWarning(GRAPH_LOG) << "configured master resource" << pinned << "is not running";
            return false;
        }
    } else if (candidates.size() == 1) {
        service = candidates.constFirst();
        // Remember the choice so a second account added later cannot silently
        // re-route this transport to the wrong mailbox.
        group.writeEntry("MasterResource", service.mid(serviceBase.size()));
        group.sync();
    } else {
        qCWarning(GRAPH_LOG) << (candidates.isEmpty() ? "no Graph resource is running" : "several Graph resources are running")
                             << "and no MasterResource is configured in" << config()->name();
        return false;
    }

    auto iface = new OrgKdeAkonadiGraphResourceInterface(service, QStringLiteral("/"), QDBusConnection::sessionBus(), this);
    if (!iface->isValid()) {
        delete iface;
        return false;
    }
    mGraphResource = iface;
    connect(mGraphResource, &OrgKdeAkonadiGraphResourceInterface::messageSent, this, &GraphMtaResource::messageSent);
    return true;
}

void GraphMtaResource::sendItem(const Akonadi::Item &item)
{
    if (!connectToMasterResource()) {
        itemSent(item, TransportFailed, i18n("Unable to connect to master Microsoft 365 (Graph) resource"));
        return;
    }

    std::shared_ptr<KMime::Message> msg;
    if (item.hasPayload<std::shared_ptr<KMime::Message>>()) {
        msg = item.payload<std::shared_ptr<KMime::Message>>();
    }
    if (!msg) {
        itemSent(item, TransportFailed, i18n("Message to be sent has no MIME payload"));
        return;
    }

    // The key is an opaque correlation token for the sendMessage/messageSent D-Bus
    // round trip. Item::id is always set and unique; the remoteId belongs to whichever
    // resource owns the outbox folder and may be empty (unlike in the EWS MTA).
    const QString id = QString::number(item.id());
    mItemHash.insert(id, item);

    /* Exchange re-assembles the message from parsed MIME parts and mangles
     * quoted-printable bodies (KMail's preferred encoding) in the process.
     * Force base64 on all parts — same workaround as the EWS MTA. */
    if (msg->contents().isEmpty()) {
        msg->changeEncoding(KMime::Headers::CEbase64);
        msg->contentTransferEncoding(KMime::CreatePolicy::Create)->setEncoding(KMime::Headers::CEbase64);
    } else {
        const auto contents = msg->contents();
        for (KMime::Content *content : contents) {
            content->changeEncoding(KMime::Headers::CEbase64);
            content->contentTransferEncoding(KMime::CreatePolicy::Create)->setEncoding(KMime::Headers::CEbase64);
        }
    }
    msg->assemble();

    mGraphResource->sendMessage(id, msg->encodedContent(KMime::NewlineType::CRLF));
}

void GraphMtaResource::messageSent(const QString &id, const QString &error)
{
    const auto it = mItemHash.constFind(id);
    if (it == mItemHash.constEnd()) {
        return;
    }
    itemSent(*it, error.isEmpty() ? TransportSucceeded : TransportFailed, error);
    mItemHash.erase(it);
}

AKONADI_RESOURCE_MAIN(GraphMtaResource)

#include "moc_graphmtaresource.cpp"
