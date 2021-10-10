/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "job.h"

#include <memory>

namespace Akonadi
{
class Collection;
class Item;

namespace FileStore
{
class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT ItemMoveJob : public Job
{
    friend class AbstractJobSession;

    Q_OBJECT

public:
    ItemMoveJob(const Item &item, const Collection &targetParent, AbstractJobSession *session = nullptr);

    ~ItemMoveJob() override;

    Collection targetParent() const;

    Item item() const;

    bool accept(Visitor *visitor) override;

private:
    void handleItemMoved(const Item &item);

private:
    class Private;
    std::unique_ptr<Private> const d;
};
}
}

