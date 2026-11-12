/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "pimdataprovider_types.h"

#include <QVariantMap>

namespace Akonadi
{
class PimDataProviderServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id)

public:
    explicit PimDataProviderServer(const QString &serverId, QObject *parent = nullptr);
    ~PimDataProviderServer() override;

    [[nodiscard]] QString id() const;
    [[nodiscard]] QList<QVariantMap> getCalendars() const;

Q_SIGNALS:
    void calendarCreated(const PimDataProviderItem &calendar);
    void calendarChanged(const PimDataProviderItem &calendar);
    void calendarDeleted(const QString &serverId, const QString &calendarId);

private:
    QString mServerId;
};
} // namespace Akonadi
