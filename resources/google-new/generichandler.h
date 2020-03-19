/*
    Copyright (C) 2020  Igor Poboiko <igor.poboiko@gmail.com>

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

#ifndef GENERICHANDLER_H
#define GENERICHANDLER_H

#include <KGAPI/Types>

#include <QObject>
#include <QSharedPointer>

#include <AkonadiCore/Item>
#include <AkonadiCore/Collection>

namespace KGAPI2 {
    class Job;
}

class GoogleResource;

class GenericHandler : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<GenericHandler> Ptr;

    GenericHandler(GoogleResource* resource);
    virtual ~GenericHandler();

    virtual QString mimetype() = 0;
    virtual bool canPerformTask(const Akonadi::Item &item) = 0;

    virtual void retrieveCollections() = 0;
    virtual void retrieveItems(const Akonadi::Collection &collection) = 0;

    virtual void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) = 0;
    virtual void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers) = 0;
    virtual void itemRemoved(const Akonadi::Item &item) = 0;
    virtual void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) = 0;
    virtual void itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection);
    virtual void itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection);

    virtual void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) = 0;
    virtual void collectionChanged(const Akonadi::Collection &collection) = 0;
    virtual void collectionRemoved(const Akonadi::Collection &collection) = 0;
protected:
    GoogleResource* m_resource;
    void emitReadyStatus();
Q_SIGNALS:
    void status(int code, const QString& message = QString());
    void percent(int progress);
    void collectionsRetrieved(const Akonadi::Collection::List& collections);
};

#endif // GENERICHANDLER_H
