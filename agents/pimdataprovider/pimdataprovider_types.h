/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDBusMetaType>
#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>

Q_DECLARE_METATYPE(QList<QVariantMap>)

struct PimDataProviderItem {
    QString id;
    QVariantMap data;
};
Q_DECLARE_METATYPE(PimDataProviderItem)

using PimDataProviderItemList = QList<PimDataProviderItem>;
Q_DECLARE_METATYPE(PimDataProviderItemList)

inline QDBusArgument &operator<<(QDBusArgument &argument, const PimDataProviderItem &entry)
{
    argument.beginStructure();
    argument << entry.id << entry.data;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, PimDataProviderItem &entry)
{
    argument.beginStructure();
    argument >> entry.id >> entry.data;
    argument.endStructure();
    return argument;
}

inline void registerPimDataProviderDBusTypes()
{
    qDBusRegisterMetaType<QList<QVariantMap>>();
    qDBusRegisterMetaType<PimDataProviderItem>();
    qDBusRegisterMetaType<PimDataProviderItemList>();
}
