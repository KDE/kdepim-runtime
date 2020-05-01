/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>
                  2020  Igor Poboiko <igor.poboiko@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers) override;
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
