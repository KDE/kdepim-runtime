/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "resourcetask.h"

#include <Akonadi/Item>
#include <KDAV/DavItem>
#include <kdav_version.h>

namespace Akonadi
{
class Collection;
}
class QObject;
namespace KDAV
{
class DavJobBase;
}

/**
 * @class MoveItemsTask
 * @brief Moves items and their dependent items from collectionSrc to collectionDst
 *
 * This task does the following :
 * - TODO:FIXME
 */
class MoveItemsTask : public ResourceTask
{
public:
    explicit MoveItemsTask(ResourceStateInterface::Ptr resource,
                           const Akonadi::Item::List &items,
                           const Akonadi::Collection &collectionSrc,
                           const Akonadi::Collection &collectionDst,
                           QObject *parent = nullptr);

protected:
    void doStart() override;

private:
    // Step 1
    void doExceptionItemsFetch();
    void onExceptionItemsFetched(KJob *job, const QMap<QString, Akonadi::Item> &uidsMap);

    // Step 2
    void doItemsMove();
#if KDAV_VERSION >= QT_VERSION_CHECK(6, 30, 0)
    void onItemsMoved(KJob *job, const Akonadi::Item::List &oldItems, const Akonadi::Item::List &newItems);
#else
    void onItemCreated(KJob *job, const Akonadi::Item::List &oldItems, Akonadi::Item::List newItems, const KDAV::DavItem &newDavItem);
#endif
    void onMoveJobDone();

    // End
    void finished();

private:
    // User-provided items to delete, and associated data
    Akonadi::Item::List m_items;
    Akonadi::Collection m_collectionSrc;
    Akonadi::Collection m_collectionDst;
    std::shared_ptr<DavItemCache> m_davItemCacheSrc;
    std::shared_ptr<DavItemCache> m_davItemCacheDst;

    // Filtered items to delete
    Akonadi::Item::List m_itemsToFetch;
    QMap<QString, Akonadi::Item::List> m_itemsToMove;

    // Batch job counters
    int m_moveJobCounter = 0;

    // Error handling
    ErrorType m_error = ErrorType::NoError;
    QString m_errorMessage;
};
