/*
    SPDX-FileCopyrightText: 2011, 2012 Dan Vratil <dan@progdan.cz>
    SPDX-FileCopyrightText: 2020 Igor Pobiko <igor.poboiko@gmail.com>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef CONTACTHANDLER_H
#define CONTACTHANDLER_H

#include "generichandler.h"
#include <KGAPI/Types>

class ContactHandler : public GenericHandler
{
    Q_OBJECT
public:
    using GenericHandler::GenericHandler;

    QString mimeType() override;
    bool canPerformTask(const Akonadi::Item &item) override;
    bool canPerformTask(const Akonadi::Item::List &items) override;

    void retrieveCollections(const Akonadi::Collection &rootCollection) override;
    void retrieveItems(const Akonadi::Collection &collection) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers) override;
    void itemsRemoved(const Akonadi::Item::List &items) override;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;
    void itemsLinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection) override;
    void itemsUnlinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;
private Q_SLOTS:
    void slotItemsRetrieved(KGAPI2::Job *job);
    void slotUpdatePhotosItemsRetrieved(KJob *job);
    void retrieveContactsPhotos(const QVariant &arguments);

private:
    QString myContactsRemoteId() const;
    void setupCollection(Akonadi::Collection &collection, const KGAPI2::ContactsGroupPtr &group);
    QMap<QString, Akonadi::Collection> m_collections;
};

#endif // CONTACTHANDLER_H
