/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef __AKONADI_SERIALIZER_ADDRESSEE_H__
#define __AKONADI_SERIALIZER_ADDRESSEE_H__

#include <QtCore/QObject>

#include <akonadi/itemserializerplugin.h>
#include <kabc/vcardconverter.h>

namespace Akonadi {

class SerializerPluginAddressee : public QObject, public ItemSerializerPlugin
{
    Q_OBJECT
    Q_INTERFACES( Akonadi::ItemSerializerPlugin )

public:
    bool deserialize( Item& item, const QByteArray& label, QIODevice& data, int version );
    void serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version );
private:
    KABC::VCardConverter m_converter;
};


}

#endif
