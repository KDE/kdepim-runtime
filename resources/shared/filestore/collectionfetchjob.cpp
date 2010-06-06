/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "collectionfetchjob.h"

#include "session_p.h"

#include <akonadi/collectionfetchscope.h>

using namespace Akonadi;

class FileStore::CollectionFetchJob::Private
{
  public:
    explicit Private( FileStore::CollectionFetchJob *parent )
      : mType( FileStore::CollectionFetchJob::Base ),
        mParent( parent )
    {
    }

  public:
    FileStore::CollectionFetchJob::Type mType;
    Collection mCollection;

    CollectionFetchScope mFetchScope;

    Collection::List mCollections;

  private:
    FileStore::CollectionFetchJob *mParent;
};

FileStore::CollectionFetchJob::CollectionFetchJob( const Collection &collection, Type type, FileStore::AbstractJobSession *session )
  : FileStore::Job( session ), d( new Private( this ) )
{
  Q_ASSERT( session != 0 );

  d->mType = type;
  d->mCollection = collection;

  session->addJob( this );
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

void FileStore::CollectionFetchJob::setFetchScope( const CollectionFetchScope &fetchScope )
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

bool FileStore::CollectionFetchJob::accept( FileStore::Job::Visitor *visitor )
{
  return visitor->visit( this );
}

void FileStore::CollectionFetchJob::handleCollectionsReceived( const Collection::List &collections )
{
  d->mCollections << collections;

  emit collectionsReceived( collections );
}

#include "collectionfetchjob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
