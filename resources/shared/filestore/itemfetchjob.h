/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2009, 2010 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FILESTORE_ITEMFETCHJOB_H
#define AKONADI_FILESTORE_ITEMFETCHJOB_H

#include "job.h"

#include <item.h>

namespace Akonadi {
class Collection;
class ItemFetchScope;

namespace FileStore {
class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT ItemFetchJob : public Job
{
    friend class AbstractJobSession;

    Q_OBJECT

public:
    explicit ItemFetchJob(const Collection &collection, AbstractJobSession *session = nullptr);

    explicit ItemFetchJob(const Item &item, AbstractJobSession *session = nullptr);

    explicit ItemFetchJob(const Item::List &items, AbstractJobSession *session = nullptr);

    ~ItemFetchJob() override;

    Collection collection() const;

    Item item() const;

    Item::List requestedItems() const;

    void setFetchScope(const ItemFetchScope &fetchScope);

    ItemFetchScope &fetchScope();

    Item::List items() const;

    bool accept(Visitor *visitor) override;

Q_SIGNALS:
    void itemsReceived(const Akonadi::Item::List &items);

private:
    void handleItemsReceived(const Akonadi::Item::List &items);

private:
    class Private;
    Private *const d;
};
}
}

#endif
