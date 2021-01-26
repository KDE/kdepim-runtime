/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionfetchjob.h"

#include "session_p.h"

#include <collectionfetchscope.h>

using namespace Akonadi;

class FileStore::CollectionFetchJob::Private
{
public:
    Private()
        : mType(FileStore::CollectionFetchJob::Base)
    {
    }

    FileStore::CollectionFetchJob::Type mType;
    Collection mCollection;

    CollectionFetchScope mFetchScope;

    Collection::List mCollections;
};

FileStore::CollectionFetchJob::CollectionFetchJob(const Collection &collection, Type type, FileStore::AbstractJobSession *session)
    : FileStore::Job(session)
    , d(new Private())
{
    Q_ASSERT(session != nullptr);

    d->mType = type;
    d->mCollection = collection;

    session->addJob(this);
}

FileStore::CollectionFetchJob::~CollectionFetchJob()
{
    delete d;
}

FileStore::CollectionFetchJob::Type FileStore::CollectionFetchJob::type() const
{
    return d->mType;
}

Collection FileStore::CollectionFetchJob::collection() const
{
    return d->mCollection;
}

void FileStore::CollectionFetchJob::setFetchScope(const CollectionFetchScope &fetchScope)
{
    d->mFetchScope = fetchScope;
}

CollectionFetchScope &FileStore::CollectionFetchJob::fetchScope()
{
    return d->mFetchScope;
}

Collection::List FileStore::CollectionFetchJob::collections() const
{
    return d->mCollections;
}

bool FileStore::CollectionFetchJob::accept(FileStore::Job::Visitor *visitor)
{
    return visitor->visit(this);
}

void FileStore::CollectionFetchJob::handleCollectionsReceived(const Collection::List &collections)
{
    d->mCollections << collections;

    Q_EMIT collectionsReceived(collections);
}
