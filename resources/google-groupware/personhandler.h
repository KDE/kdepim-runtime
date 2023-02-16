/*
    SPDX-FileCopyrightText: 2011-2013 Daniel Vrátil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2020 Igor Pobiko <igor.poboiko@gmail.com>
    SPDX-FileCopyrightText: 2022-2023 Claudio Cambra <claudio.cambra@kde.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "generichandler.h"
#include <KGAPI/Types>

class PersonHandler : public GenericHandler
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
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override {};
    void itemsLinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection) override {};
    void itemsUnlinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection) override {};

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override {};
    void collectionChanged(const Akonadi::Collection &collection) override {};
    void collectionRemoved(const Akonadi::Collection &collection) override {};

private:
    void slotItemsRetrieved(KGAPI2::Job *job);
    void slotPersonCreateJobFinished(KGAPI2::Job *job);
    void slotPersonModifyJobFinished(KGAPI2::Job *job);
    void processUpdatedPerson(KGAPI2::Job *job, const KGAPI2::People::PersonPtr &person);

    static Akonadi::Collection collectionFromContactGroup(const KGAPI2::People::ContactGroupPtr &group);
    static QString addresseeMimeType();

    QMap<QString, Akonadi::Collection> m_collections;
};
