/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QSharedPointer>
#include <QStringList>

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/ItemSync>

class Settings;
class DavItemCache;

class ResourceStateInterface
{
public:
    using Ptr = QSharedPointer<ResourceStateInterface>;

    virtual ~ResourceStateInterface();

    virtual QString resourceName() const = 0;
    virtual QString resourceIdentifier() const = 0;

    virtual void itemRetrieved(const Akonadi::Item &item) = 0;
    virtual void itemsRetrieved(const Akonadi::Item::List &items) = 0;
    virtual void setTotalItems(int) = 0;
    virtual void itemsRetrievedIncremental(const Akonadi::Item::List &changed, const Akonadi::Item::List &removed) = 0;
    virtual void itemsRetrievalDone() = 0;

    virtual void collectionsRetrieved(const Akonadi::Collection::List &collections) = 0;
    virtual void collectionAttributesRetrieved(const Akonadi::Collection &collection) = 0;

    virtual void itemChangeCommitted(const Akonadi::Item &item) = 0;
    virtual void itemsChangesCommitted(const Akonadi::Item::List &items) = 0;
    virtual void collectionChangeCommitted(const Akonadi::Collection &collection) = 0;

    virtual void changeProcessed() = 0;
    virtual void taskDone() = 0;
    virtual void cancelTask() = 0;
    virtual void cancelTask(const QString &errorString) = 0;
    virtual void deferTask() = 0;
    virtual void retryAfterFailure(const QString &errorMessage) = 0;

    virtual void emitError(const QString &message) = 0;
    virtual void emitWarning(const QString &message) = 0;
    virtual void emitPercent(int percent) = 0;

    virtual void synchronizeCollection(Akonadi::Collection::Id) = 0;
    virtual void synchronizeCollectionTree() = 0;

    virtual Settings *settings() = 0;
    virtual auto davItemCache() -> QMap<QString, std::shared_ptr<DavItemCache>> = 0;
};
