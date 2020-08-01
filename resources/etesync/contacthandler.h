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

#ifndef CONTACTHANDLER_H
#define CONTACTHANDLER_H

#include <kcontacts/addressee.h>

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>

#include "basehandler.h"
#include "etesyncadapter.h"
#include "etesyncclientstate.h"

class EteSyncResource;

class ContactHandler : public BaseHandler
{
    Q_OBJECT
public:
    explicit ContactHandler(EteSyncResource *resource);

    const QString mimeType() override;

    void setupItems(EteSyncEntry **entries, Akonadi::Collection &collection) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

protected:
    QString getLocalContact(QString contactUid) const;
    void updateLocalContact(const KContacts::Addressee &contact);
    void deleteLocalContact(const KContacts::Addressee &contact);

    QString baseDirectoryPath() const override;

    const QString etesyncCollectionType() override;
};

#endif
