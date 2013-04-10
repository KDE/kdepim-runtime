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

  object.setId( map["id"].toString() );
  object.setFrom( map["from"].toMap() );
  object.setTo( map["to"].toMap() );
  object.setCreatedTimeString( map["created_time"].toString() );
  object.setUpdatedTimeString( map["updated_time"].toString() );
  object.setTitle( map["title"].toString() );
  object.setMessage( map["message"].toString() );
  object.setLink( map["link"].toUrl() );
  object.setApplication( map["application"].toMap() );
  object.setUnread( map["unread"].toBool() );

  item.setMimeType( "text/x-vnd.akonadi.socialnotification" );
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

  map["id"] = notificationInfo.id();

  QVariantMap fromMap;
  if ( !notificationInfo.from().id().isEmpty() ) {
    fromMap["name"] = notificationInfo.from().name();
    fromMap["id"] = notificationInfo.from().id();
  }

  map["from"] = fromMap;

  QVariantMap toMap;
  if ( !notificationInfo.to().id().isEmpty() ) {
    toMap["name"] = notificationInfo.to().name();
    toMap["id"] = notificationInfo.to().id();
  }

  map["to"] = toMap;
  map["created_time"] = notificationInfo.createdTimeString();
  map["updated_time"] = notificationInfo.updatedTimeString();
  map["title"] = notificationInfo.title();
  map["message"] = notificationInfo.message();
  map["link"] = notificationInfo.link();

  QVariantMap appMap;
  if ( !notificationInfo.application().id().isEmpty() ) {
    appMap["name"] = notificationInfo.application().name();
    appMap["id"] = notificationInfo.application().id();
  }

  map["app"] = appMap;
  map["unread"] = notificationInfo.unread();

  QJson::Serializer serializer;

  data.write( serializer.serialize( map ) );
}

Q_EXPORT_PLUGIN2( akonadi_serializer_socialnotification,
                  Akonadi::SerializerPluginSocialNotification )
