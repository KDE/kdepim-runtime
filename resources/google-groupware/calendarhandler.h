/*
    Copyright (C) 2011-2013  Daniel Vrátil <dvratil@redhat.com>
                  2020  Igor Pobiko <igor.poboiko@gmail.com>

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

#ifndef CALENDARHANDLER_H
#define CALENDARHANDLER_H

#include "generichandler.h"
#include <QObject>
#include <QSharedPointer>

class CalendarHandler : public GenericHandler
{
    Q_OBJECT
public:
    typedef QSharedPointer<CalendarHandler> Ptr;

    using GenericHandler::GenericHandler;

    QString mimetype() override;
    bool canPerformTask(const Akonadi::Item &item) override;

    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers) override;
    void itemsRemoved(const Akonadi::Item::List &items) override;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;
    // FreeBusy
    QDateTime lastCacheUpdate() const;
    void canHandleFreeBusy(const QString &email) const;
    void retrieveFreeBusy(const QString &email, const QDateTime &start, const QDateTime &end);
private Q_SLOTS:
    void slotCollectionsRetrieved(KGAPI2::Job* job);
    void slotItemsRetrieved(KGAPI2::Job* job);
private:
    void setupCollection(Akonadi::Collection& collection, const KGAPI2::CalendarPtr& group);
};

#endif // CALENDARHANDLER_H
