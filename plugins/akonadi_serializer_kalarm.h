/*
 *  akonadi_serializer_kalarm.h  -  Akonadi resource serializer for KAlarm
 *  Copyright © 2009-2014 by David Jarvie <djarvie@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#ifndef AKONADI_SERIALIZER_KALARM_H
#define AKONADI_SERIALIZER_KALARM_H

#include "kaeventformatter.h"

#include <AkonadiCore/itemserializerplugin.h>
#include <AkonadiCore/differencesalgorithminterface.h>
#include <AkonadiCore/gidextractorinterface.h>
#include <AkonadiCore/IndexerInterface>
#include <AkonadiSearch/ObjectCache>
#include <KCalCore/ICalFormat>

#include <QObject>

namespace Akonadi {
class Item;
class AbstractDifferencesReporter;

class SerializerPluginKAlarm : public QObject
                             , public ItemSerializerPlugin
                             , public DifferencesAlgorithmInterface
                             , public GidExtractorInterface
                             , public ItemIndexerInterface
{
    Q_OBJECT
    Q_INTERFACES(Akonadi::ItemSerializerPlugin
                 Akonadi::DifferencesAlgorithmInterface
                 Akonadi::GidExtractorInterface
                 Akonadi::ItemIndexerInterface)
    Q_PLUGIN_METADATA(IID "org.kde.akonadi.SerializerPluginKAlarm")
public:
    bool deserialize(Item &item, const QByteArray &label, QIODevice &data, int version) override;
    void serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version) override;
    void compare(AbstractDifferencesReporter *, const Item &left, const Item &right) override;
    QString extractGid(const Item &item) const override;
    QByteArray index(const Item &item, const Collection &parent) const override;

private:
    void reportDifference(AbstractDifferencesReporter *, KAEventFormatter::Parameter);

    KCalCore::ICalFormat mFormat;
    KAEventFormatter mValueL;
    KAEventFormatter mValueR;
    QString mRegistered;
    Search::IndexerCache mIndexer;
};

}

#endif // AKONADI_SERIALIZER_KALARM_H
