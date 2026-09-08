/*
    SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KUnifiedPush/Connector>
#include <QDBusContext>

class ResourceHandler : public QObject, public QDBusContext
{
    Q_OBJECT
public:
    explicit ResourceHandler(const QString &resourceName, const QString &vapid, QObject *parent = nullptr);
    [[nodiscard]] QString vapid() const;
    void setVapid(const QString &vapid);

private:
    QString m_resourceName;
    QString m_vapid;
    std::unique_ptr<KUnifiedPush::Connector> m_connector;
};
