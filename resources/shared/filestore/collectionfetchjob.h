/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2009, 2010 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FILESTORE_COLLECTIONFETCHJOB_H
#define AKONADI_FILESTORE_COLLECTIONFETCHJOB_H

#include "job.h"

#include <collection.h>

namespace Akonadi
{
class CollectionFetchScope;

namespace FileStore
{
class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT CollectionFetchJob : public Job
{
    friend class AbstractJobSession;

    Q_OBJECT

public:
    enum Type { Base, FirstLevel, Recursive };

    explicit CollectionFetchJob(const Collection &collection, Type type = FirstLevel, AbstractJobSession *session = nullptr);

    ~CollectionFetchJob() override;

    Type type() const;

    Collection collection() const;

    void setFetchScope(const CollectionFetchScope &fetchScope);

    CollectionFetchScope &fetchScope();

    Collection::List collections() const;

    bool accept(Visitor *visitor) override;

Q_SIGNALS:
    void collectionsReceived(const Akonadi::Collection::List &items);

private:
    void handleCollectionsReceived(const Akonadi::Collection::List &collections);

private:
    class Private;
    Private *const d;
};
}
}

#endif
