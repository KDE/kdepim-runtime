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

#ifndef AKONADI_SERIALIZER_SOCIALNOTIFICATION_H
#define AKONADI_SERIALIZER_SOCIALNOTIFICATION_H

#include <Akonadi/ItemSerializerPlugin>
#include <QObject>

namespace Akonadi {

class SerializerPluginSocialNotification : public QObject, public ItemSerializerPlugin
{
  Q_OBJECT
  Q_INTERFACES( Akonadi::ItemSerializerPlugin )

  public:
    bool deserialize( Item &item, const QByteArray &label, QIODevice &data, int version );
    void serialize( const Item &item, const QByteArray &label, QIODevice &data, int &version );
};

}

#endif // AKONADI_SERIALIZER_SOCIALNOTIFICATION_H
