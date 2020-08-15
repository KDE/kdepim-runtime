/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2009, 2010 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FILESTORE_ITEMDELETEJOB_H
#define AKONADI_FILESTORE_ITEMDELETEJOB_H

#include "job.h"

namespace Akonadi {
class Item;

namespace FileStore {
class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT ItemDeleteJob : public Job
{
    friend class AbstractJobSession;

    Q_OBJECT

public:
    explicit ItemDeleteJob(const Item &item, AbstractJobSession *session = nullptr);

    ~ItemDeleteJob() override;

    Item item() const;

    bool accept(Visitor *visitor) override;

private:
    void handleItemDeleted(const Akonadi::Item &item);

private:
    class Private;
    Private *const d;
};
}
}

#endif
