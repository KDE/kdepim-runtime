/*
    Copyright (C) 2008    Dmitry Ivanov <vonami@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AKONADI_SERIALIZER_RSS
#define AKONADI_SERIALIZER_RSS

#include <krss/rssitemserializer.h>

#include <akonadi/itemserializerplugin.h>

class QIODevice;

class SerializerPluginRss : public QObject, public Akonadi::ItemSerializerPlugin
{
    Q_OBJECT
    Q_INTERFACES( Akonadi::ItemSerializerPlugin )

public:
    bool deserialize( Akonadi::Item& item, const QByteArray& label, QIODevice& data, int version);
    void serialize( const Akonadi::Item& item, const QByteArray& label, QIODevice& data, int& version);
    QSet<QByteArray> parts( const Akonadi::Item& item ) const;

private:
#ifdef KRSS_ENABLE_PROTOBUF_SERIALIZER
    KRss::ProtobufRssItemSerializerImpl m_serializer;
#else
    KRss::RssItemSerializer m_serializer;
#endif
};

#endif /* AKONADI_SERIALIZER_RSS */
