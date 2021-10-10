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

namespace FileStore
{
class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT CollectionModifyJob : public Job
{
    friend class AbstractJobSession;

    Q_OBJECT

public:
    explicit CollectionModifyJob(const Collection &collection, AbstractJobSession *session = nullptr);

    ~CollectionModifyJob() override;

    Collection collection() const;

    bool accept(Visitor *visitor) override;

private:
    void handleCollectionModified(const Collection &collection);

private:
    class Private;
    std::unique_ptr<Private> const d;
};
}
}

