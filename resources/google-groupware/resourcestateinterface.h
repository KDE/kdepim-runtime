/*
    Copyright (c) 2020 Igor Poboiko <igor.poboiko@gmail.com>
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef RESOURCESTATEINTERFACE_H
#define RESOURCESTATEINTERFACE_H

#include <Collection>
#include <Item>
#include <ItemSync>

/**
 * This is a generic interface for ResourceBase class
 */
class ResourceStateInterface
{
public:
    typedef QSharedPointer<ResourceStateInterface> Ptr;

    virtual ~ResourceStateInterface() = default;

    // Items handling
    virtual void itemRetrieved(const Akonadi::Item &item) = 0;
    virtual void itemsRetrieved(const Akonadi::Item::List &items) = 0;
    virtual void itemsRetrievedIncremental(const Akonadi::Item::List &changed, const Akonadi::Item::List &removed) = 0;
    virtual void itemsRetrievalDone() = 0;
    virtual void setTotalItems(int) = 0;
    virtual void itemChangeCommitted(const Akonadi::Item &item) = 0;
    virtual void itemsChangesCommitted(const Akonadi::Item::List &items) = 0;
    virtual Akonadi::Item::List currentItems() = 0;

    // Collections handling
    virtual void collectionsRetrieved(const Akonadi::Collection::List &collections) = 0;
    virtual void collectionAttributesRetrieved(const Akonadi::Collection &collection) = 0;
    virtual void collectionChangeCommitted(const Akonadi::Collection &collection) = 0;
    virtual Akonadi::Collection currentCollection() = 0;

    // Tags handling
    virtual void tagsRetrieved(const Akonadi::Tag::List &tags, const QHash<QString, Akonadi::Item::List> &) = 0;
    virtual void tagChangeCommitted(const Akonadi::Tag &tag) = 0;

    // Relations handling
    virtual void relationsRetrieved(const Akonadi::Relation::List &tags) = 0;

    // Result reporting
    virtual void changeProcessed() = 0;
    virtual void cancelTask(const QString &errorString) = 0;
    virtual void deferTask() = 0;
    virtual void taskDone() = 0;

    virtual void emitStatus(int status, const QString &message) = 0;
    virtual void emitError(const QString &message) = 0;
    virtual void emitWarning(const QString &message) = 0;
    virtual void emitPercent(int percent) = 0;
};

#endif // RESOURCESTATEINTERFACE_H
