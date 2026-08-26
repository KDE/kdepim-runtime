/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "resourcetask.h"

#include <Akonadi/Item>

/**
 * @class RemoveItemsTask
 * @brief Removes items and their dependent items from Dav server
 *
 * This task does the following :
 * - filters items to delete if it's the main item (no recurring exception),
 * - identify exceptions of these items,
 * - remove these exceptions from akonadi,
 * - delete the main items,
 * and
 * - filters items whom we are removing an exception,
 * - fetch from akonadiserver the concerned items,
 * - update these items whose recurring exceptions changed.
 */
class RemoveItemsTask : public ResourceTask
{
public:
    explicit RemoveItemsTask(DavGroupwareResource *resource, Akonadi::Item::List items, QObject *parent = nullptr);

protected:
    void doStart() override;

private:
    void doItemsFetch();
    void onItemsFetched(KJob *job);

    void doItemsChange(const QMap<QString, Akonadi::Item::List> &itemsOccurrences);
    void onItemChanged(KJob *job, const QString &ridBase);

    void doItemsExceptionDeletion();

    void doItemDeletion();
    void onItemDeleted(KJob *job, const Akonadi::Item &item);

    void finished();

private:
    // User-provided items to delete, and associated data
    Akonadi::Item::List m_items;
    Akonadi::Collection m_collection;
    std::shared_ptr<DavItemCache> m_davItemCache;

    // Filtered items to delete
    Akonadi::Item::List m_itemsToDelete;
    QMap<QString, Akonadi::Item::List> m_occurrencesToDelete;

    // Batch job counters
    int m_davItemChangeJobCounter = 0;
    int m_davItemDeleteJobCounter = 0;
};
