/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BASEHANDLER_H
#define BASEHANDLER_H

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>
#include <KCalendarCore/Incidence>

#include "etesyncadapter.h"
#include "etesyncclientstate.h"

using namespace Akonadi;

class EteSyncResource;

class BaseHandler : public QObject
{
    Q_OBJECT
public:
    typedef std::unique_ptr<BaseHandler> Ptr;
    explicit BaseHandler(EteSyncResource *resource);

    virtual const QString mimeType() = 0;

    virtual void setupItems(EteSyncEntry **entries, Akonadi::Collection &collection) = 0;

    virtual void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) = 0;
    virtual void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) = 0;
    virtual void itemRemoved(const Akonadi::Item &item) = 0;

    virtual void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) = 0;
    virtual void collectionChanged(const Akonadi::Collection &collection) = 0;
    virtual void collectionRemoved(const Akonadi::Collection &collection) = 0;

protected:
    void initialiseBaseDirectory();
    void createEteSyncEntry(const EteSyncSyncEntry *syncEntry, const EteSyncCryptoManager *cryptoManager, const Collection &collection);
    void updateCollectionRevision(const EteSyncEntry *entry, const Collection &collection);

    virtual QString baseDirectoryPath() const = 0;
    virtual const QString etesyncCollectionType() = 0;

    EteSyncResource *mResource = nullptr;
    EteSyncClientState *mClientState = nullptr;
};

#endif
