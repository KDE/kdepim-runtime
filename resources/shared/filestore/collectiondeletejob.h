/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FILESTORE_COLLECTIONDELETEJOB_H
#define AKONADI_FILESTORE_COLLECTIONDELETEJOB_H

#include "job.h"

namespace Akonadi {
class Collection;

namespace FileStore {
class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT CollectionDeleteJob : public Job
{
    friend class AbstractJobSession;

    Q_OBJECT

public:
    explicit CollectionDeleteJob(const Collection &collection, AbstractJobSession *session = nullptr);

    ~CollectionDeleteJob() override;

    Collection collection() const;

    bool accept(Visitor *visitor) override;

private:
    void handleCollectionDeleted(const Collection &collection);

private:
    class Private;
    Private *const d;
};
}
}

#endif
