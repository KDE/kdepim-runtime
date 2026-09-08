/*
SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "resourcehandler.h"

#include "davpushnotifybridge_debug.h"
#include <QString>
#include <qlatin1stringview.h>
#include <qloggingcategory.h>

using namespace Qt::Literals;

ResourceHandler::ResourceHandler(const QString &resourceName, const QString &vapid, QObject *parent)
    : QObject(parent)
    , m_resourceName(resourceName)
    , m_vapid(vapid)
{
    m_connector = std::make_unique<KUnifiedPush::Connector>("org.kde.akonadi_davpushnotifybridge"_L1, resourceName);
    m_connector->setVapidPublicKey(m_vapid);
    m_connector->setVapidPublicKeyRequired(true);
    connect(m_connector.get(), &KUnifiedPush::Connector::messageReceived, this, [this](const QByteArray &data) {
        qCDebug(DAVPUSHNOTIFYBRIDGE_LOG) << this->vapid() << "DAVPUSH Message received" << data;
    });
    connect(m_connector.get(), &KUnifiedPush::Connector::endpointChanged, this, [this](const QString &endpoint) {
        qCDebug(DAVPUSHNOTIFYBRIDGE_LOG) << this->vapid() << "Endpoint changed" << endpoint;
    });
    connect(m_connector.get(), &KUnifiedPush::Connector::stateChanged, this, [this](auto state) {
        qCDebug(DAVPUSHNOTIFYBRIDGE_LOG) << "stateChanged" << this->vapid() << "State" << state;
    });
    m_connector->registerClient(vapid);
}

QString ResourceHandler::vapid() const
{
    return m_vapid;
}

void ResourceHandler::setVapid(const QString &vapid)
{
    m_vapid = vapid;
    m_connector->setVapidPublicKey(m_vapid);
}
