/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "resourcestateinterface.h"

class DavGroupwareResource;

class ResourceState : public ResourceStateInterface
{
public:
    explicit ResourceState(DavGroupwareResource *resource);

public:
    ~ResourceState() override;

    QString resourceName() const override;
    QString resourceIdentifier() const override;

    void itemRetrieved(const Akonadi::Item &item) override;
    void itemsRetrieved(const Akonadi::Item::List &items) override;
    void setTotalItems(int) override;
    void itemsRetrievedIncremental(const Akonadi::Item::List &changed, const Akonadi::Item::List &removed) override;
    void itemsRetrievalDone() override;

    void collectionsRetrieved(const Akonadi::Collection::List &collections) override;
    void collectionAttributesRetrieved(const Akonadi::Collection &collection) override;

    void itemChangeCommitted(const Akonadi::Item &item) override;
    void itemsChangesCommitted(const Akonadi::Item::List &items) override;
    void collectionChangeCommitted(const Akonadi::Collection &collection) override;

    void changeProcessed() override;
    void taskDone() override;
    void cancelTask() override;
    void cancelTask(const QString &errorString) override;
    void deferTask() override;
    void retryAfterFailure(const QString &message) override;

    void emitError(const QString &message) override;
    void emitWarning(const QString &message) override;
    void emitPercent(int percent) override;

    void synchronizeCollection(Akonadi::Collection::Id) override;
    void synchronizeCollectionTree() override;

    auto settings() -> Settings * override;
    auto davItemCache() -> QMap<QString, std::shared_ptr<DavItemCache>> override;

private:
    DavGroupwareResource *const m_resource;
};
