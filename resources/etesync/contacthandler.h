/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
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

    void getItemListFromEntries(std::vector<EteSyncEntryPtr> &entries, Item::List &changedItems, Item::List &removedItems, Collection &collection, const QString &journalUid, QString &prevUid) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

protected:
    QString getLocalContact(const QString &contactUid) const;
    void updateLocalContact(const KContacts::Addressee &contact);
    void deleteLocalContact(const QString &incidenceUid);

    QString baseDirectoryPath() const override;

    const QString etesyncCollectionType() override;
};

#endif
