/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

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

#ifndef __AKONADI_SERIALIZER_COLLECTION_H_
#define __AKONADI_SERIALIZER_COLLECTION_H_

#include <QObject>

#include <AkonadiCore/ItemSerializerPlugin>
#include <AkonadiCore/IndexerInterface>
#include <AkonadiSearch/ObjectCache>

namespace Akonadi {

class SerializerPluginCollection : public QObject
                                 , public ItemSerializerPlugin
                                 , public CollectionIndexerInterface
{
    Q_OBJECT
    Q_INTERFACES(Akonadi::ItemSerializerPlugin
                 Akonadi::CollectionIndexerInterface)
    Q_PLUGIN_METADATA(IID "org.kde.akonadi.SerializerPluginCollection")
public:
    bool deserialize(Item &item, const QByteArray &label, QIODevice &data, int version) override;
    void serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version) override;

    QByteArray index(const Collection &collection, const Collection &parent) const override;

private:
    Search::IndexerCache m_indexer;
};
}

#endif
