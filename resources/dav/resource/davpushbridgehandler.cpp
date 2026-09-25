/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "davpushbridgehandler.h"

#include "davresource_debug.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QUrl>

using namespace Qt::Literals;

static constexpr auto DAVPUSH_SERVICE = "org.kde.akonadi_davpushnotifybridge"_L1;
static constexpr auto DAVPUSH_PATH = "/Management"_L1;
static constexpr auto DAVPUSH_INTERFACE = "org.kde.akonadi_davpushnotifybridge.Management"_L1;

DavPushBridgeHandler::DavPushBridgeHandler(const QString &resourceName)
    : mResourceName(resourceName)
    , mConnection(QDBusConnection::sessionBus())
{
}

void DavPushBridgeHandler::connectHandler()
{
    auto sessionBus = QDBusConnection::sessionBus();
    // TODO: check what needs to be done to be robust here
    {
        constexpr auto signalName = "endpointChanged"_L1;
        bool connected = sessionBus.connect(DAVPUSH_SERVICE,
                                            DAVPUSH_PATH,
                                            DAVPUSH_INTERFACE,
                                            signalName,
                                            this,
                                            SLOT(onEndpointChange(QString, QString, QByteArray, QByteArray)));
        Q_ASSERT(connected);
    }

    {
        constexpr auto signalName = "contentUpdate"_L1;
        bool connected =
            sessionBus.connect(DAVPUSH_SERVICE, DAVPUSH_PATH, DAVPUSH_INTERFACE, signalName, this, SLOT(onContentUpdate(QString, QString, QString)));
        Q_ASSERT(connected);
    }

    {
        constexpr auto signalName = "propertyUpdate"_L1;
        bool connected = sessionBus.connect(DAVPUSH_SERVICE, DAVPUSH_PATH, DAVPUSH_INTERFACE, signalName, this, SLOT(onPropertyUpdate(QString, QString)));
        Q_ASSERT(connected);
    }

    {
        constexpr auto signalName = "vapidKeyChanged"_L1;
        bool connected = sessionBus.connect(DAVPUSH_SERVICE, DAVPUSH_PATH, DAVPUSH_INTERFACE, signalName, this, SLOT(onVapidKeyChange(QString)));
        Q_ASSERT(connected);
    }

    qCDebug(DAVRESOURCE_LOG()) << "DavPush: Connected to" << DAVPUSH_INTERFACE << "signals";
}

void DavPushBridgeHandler::setResourceName(const QString &resourceName)
{
    mResourceName = resourceName;
}
QString DavPushBridgeHandler::resourceName() const
{
    return mResourceName;
}

QDBusPendingCall DavPushBridgeHandler::registerResource(const QString &vapidKey, const QString &resourceName)
{
    const auto &resourceIdentifier = !resourceName.isEmpty() ? resourceName : mResourceName;

    auto iface = QDBusInterface(DAVPUSH_SERVICE, DAVPUSH_PATH, DAVPUSH_INTERFACE, mConnection);
    return iface.asyncCall("registerResource"_L1, resourceIdentifier, vapidKey);
}

void DavPushBridgeHandler::onEndpointChange(const QString &resourceName,
                                            const QString &endpoint,
                                            const QByteArray &authSecret,
                                            const QByteArray &encryptionPublicKey)
{
    if (resourceName != mResourceName) {
        qCDebug(DAVRESOURCE_LOG()) << "DavPush: Ignoring push endpoint changed for" << resourceName;
        return;
    }

    const auto endpointUrl = QUrl::fromUserInput(endpoint);
    if (!endpointUrl.isValid()) {
        qCWarning(DAVRESOURCE_LOG()) << "DavPush: Invalid push endpoint url:" << endpoint;
        return;
    }

    Q_EMIT endpointChanged(endpointUrl,
                           authSecret.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals),
                           encryptionPublicKey.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

void DavPushBridgeHandler::onContentUpdate(const QString &resourceName, const QString &topic, const QString &syncToken)
{
    if (resourceName != mResourceName) {
        qCDebug(DAVRESOURCE_LOG()) << "Ignoring push content update for" << resourceName;
        return;
    }

    Q_EMIT contentUpdated(topic, syncToken);
}

void DavPushBridgeHandler::onPropertyUpdate(const QString &resourceName, const QString &topic)
{
    if (resourceName != mResourceName) {
        qCDebug(DAVRESOURCE_LOG()) << "Ignoring push property update for" << resourceName;
        return;
    }

    Q_EMIT propertyUpdated(topic);
}

void DavPushBridgeHandler::onVapidKeyChange(const QString &resourceName)
{
    if (resourceName != mResourceName) {
        qCDebug(DAVRESOURCE_LOG()) << "Ignoring push vapid key update for" << resourceName;
        return;
    }

    Q_EMIT vapidKeyChanged();
}

#include "moc_davpushbridgehandler.cpp"
