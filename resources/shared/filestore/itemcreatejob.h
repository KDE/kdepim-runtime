/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2009, 2010 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FILESTORE_ITEMCREATEJOB_H
#define AKONADI_FILESTORE_ITEMCREATEJOB_H

#include "job.h"

namespace Akonadi
{
class Collection;
class Item;

namespace FileStore
{
class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT ItemCreateJob : public Job
{
    friend class AbstractJobSession;

    Q_OBJECT

public:
    explicit ItemCreateJob(const Item &item, const Collection &collection, AbstractJobSession *session = nullptr);

    ~ItemCreateJob() override;

    Collection collection() const;

    Item item() const;

    bool accept(Visitor *visitor) override;

private:
    void handleItemCreated(const Akonadi::Item &item);

private:
    class Private;
    Private *const d;
};
}
}

#endif
