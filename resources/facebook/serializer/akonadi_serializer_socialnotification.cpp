/*
  Copyright (C) 2012  Martin Klapetek <martin.klapetek@gmail.com>

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published
  by the Free Software Foundation; either version 2 of the License or
  ( at your option ) version 3 or, at the discretion of KDE e.V.
  ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "akonadi_serializer_socialnotification.h"

#include <KFbAPI/postinfo.h>
#include <KFbAPI/notificationinfo.h>
#include <KFbAPI/userinfo.h>

#include <AkonadiCore/Item>

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Akonadi;

bool SerializerPluginSocialNotification::deserialize(Item &item, const QByteArray &label,
        QIODevice &data, int version)
{
    Q_UNUSED(version)

    if (label != Item::FullPayload) {
        return false;
    }

    QJsonDocument json = QJsonDocument::fromJson(data.readAll());
    QJsonObject map = json.object();

    KFbAPI::NotificationInfo object(map);

    item.setMimeType(QStringLiteral("text/x-vnd.akonadi.socialnotification"));
    item.setPayload<KFbAPI::NotificationInfo>(object);

    return true;
}

void SerializerPluginSocialNotification::serialize(const Item &item, const QByteArray &label,
        QIODevice &data, int &version)
{
    Q_UNUSED(label)
    Q_UNUSED(version)

    if (!item.hasPayload< KFbAPI::NotificationInfo >()) {
        return;
    }

    KFbAPI::NotificationInfo notificationInfo = item.payload< KFbAPI::NotificationInfo >();

    QJsonObject map;

    map.insert(QStringLiteral("id"), notificationInfo.id());

    QJsonObject from;
    if (!notificationInfo.from().id().isEmpty()) {
        from.insert(QStringLiteral("name"), notificationInfo.from().name());
        from.insert(QStringLiteral("id"), notificationInfo.from().id());
    }

    map.insert(QStringLiteral("from"), from);

    QJsonObject to;
    if (!notificationInfo.to().id().isEmpty()) {
        to.insert(QStringLiteral("name"), notificationInfo.to().name());
        to.insert(QStringLiteral("id"), notificationInfo.to().id());
    }

    map.insert(QStringLiteral("to"), to);
    map.insert(QStringLiteral("created_time"), notificationInfo.createdTimeString());
    map.insert(QStringLiteral("updated_time"), notificationInfo.updatedTimeString());
    map.insert(QStringLiteral("title"), notificationInfo.title());
    map.insert(QStringLiteral("link"), notificationInfo.link().toString());

    QJsonObject app;
    if (!notificationInfo.application().id().isEmpty()) {
        app.insert(QStringLiteral("name"), notificationInfo.application().name());
        app.insert(QStringLiteral("id"), notificationInfo.application().id());
    }

    map.insert(QStringLiteral("app"), app);
    map.insert(QStringLiteral("unread"), notificationInfo.unread());

    data.write(QJsonDocument(map).toJson(QJsonDocument::Indented));
}
