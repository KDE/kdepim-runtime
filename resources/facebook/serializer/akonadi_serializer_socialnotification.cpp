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

#include <libkfbapi/postinfo.h>
#include <libkfbapi/notificationinfo.h>
#include <libkfbapi/userinfo.h>

#include <QtCore/qplugin.h>

#include <Akonadi/Item>
#include <qjson/serializer.h>
#include <qjson/parser.h>

#include <KDebug>

using namespace Akonadi;

bool SerializerPluginSocialNotification::deserialize( Item &item, const QByteArray &label,
                                                      QIODevice &data, int version )
{
  Q_UNUSED( version )

  if ( label != Item::FullPayload ) {
    return false;
  }

  KFbAPI::NotificationInfo object;

  //FIXME: Use   QJson::QObjectHelper::qvariant2qobject( item.toMap(), postInfo.data() );
  QJson::Parser parser;
  QVariantMap map = parser.parse( data.readAll() ).toMap();

  object.setId( map[QLatin1String("id")].toString() );
  object.setFrom( map[QLatin1String("from")].toMap() );
  object.setTo( map[QLatin1String("to")].toMap() );
  object.setCreatedTimeString( map[QLatin1String("created_time")].toString() );
  object.setUpdatedTimeString( map[QLatin1String("updated_time")].toString() );
  object.setTitle( map[QLatin1String("title")].toString() );
  object.setMessage( map[QLatin1String("message")].toString() );
  object.setLink( map[QLatin1String("link")].toUrl() );
  object.setApplication( map[QLatin1String("application")].toMap() );
  object.setUnread( map[QLatin1String("unread")].toBool() );

  item.setMimeType( QLatin1String("text/x-vnd.akonadi.socialnotification") );
  item.setPayload< KFbAPI::NotificationInfo >( object );

  return true;
}

void SerializerPluginSocialNotification::serialize( const Item &item, const QByteArray &label,
                                                    QIODevice &data, int &version )
{
  Q_UNUSED( label )
  Q_UNUSED( version )

  if ( !item.hasPayload< KFbAPI::NotificationInfo >() ) {
    return;
  }

  KFbAPI::NotificationInfo notificationInfo = item.payload< KFbAPI::NotificationInfo >();

  QVariantMap map;

  map[QLatin1String("id")] = notificationInfo.id();

  QVariantMap fromMap;
  if ( !notificationInfo.from().id().isEmpty() ) {
    fromMap[QLatin1String("name")] = notificationInfo.from().name();
    fromMap[QLatin1String("id")] = notificationInfo.from().id();
  }

  map[QLatin1String("from")] = fromMap;

  QVariantMap toMap;
  if ( !notificationInfo.to().id().isEmpty() ) {
    toMap[QLatin1String("name")] = notificationInfo.to().name();
    toMap[QLatin1String("id")] = notificationInfo.to().id();
  }

  map[QLatin1String("to")] = toMap;
  map[QLatin1String("created_time")] = notificationInfo.createdTimeString();
  map[QLatin1String("updated_time")] = notificationInfo.updatedTimeString();
  map[QLatin1String("title")] = notificationInfo.title();
  map[QLatin1String("message")] = notificationInfo.message();
  map[QLatin1String("link")] = notificationInfo.link();

  QVariantMap appMap;
  if ( !notificationInfo.application().id().isEmpty() ) {
    appMap[QLatin1String("name")] = notificationInfo.application().name();
    appMap[QLatin1String("id")] = notificationInfo.application().id();
  }

  map[QLatin1String("app")] = appMap;
  map[QLatin1String("unread")] = notificationInfo.unread();

  QJson::Serializer serializer;

  data.write( serializer.serialize( map ) );
}

Q_EXPORT_PLUGIN2( akonadi_serializer_socialnotification,
                  Akonadi::SerializerPluginSocialNotification )
