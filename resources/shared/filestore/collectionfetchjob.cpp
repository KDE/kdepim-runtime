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

using namespace Akonadi::FileStore;

class CollectionFetchJob::Private
{
  public:
    explicit Private( CollectionFetchJob *parent )
      : mType( CollectionFetchJob::Base ),
        mParent( parent )
    {
    }

  public:
    CollectionFetchJob::Type mType;
    Collection mCollection;

    CollectionFetchScope mFetchScope;

    Collection::List mCollections;

  private:
    CollectionFetchJob *mParent;
};

CollectionFetchJob::CollectionFetchJob( const Akonadi::Collection &collection, Type type, AbstractJobSession *session )
  : Job( session ), d( new Private( this ) )
{
  Q_ASSERT( session != 0 );

  d->mType = type;
  d->mCollection = collection;

  session->addJob( this );
}

CollectionFetchJob::~CollectionFetchJob()
{
  delete d;
}

CollectionFetchJob::Type CollectionFetchJob::type() const
{
  return d->mType;
}

Akonadi::Collection CollectionFetchJob::collection() const
{
  return d->mCollection;
}

void CollectionFetchJob::setFetchScope( const Akonadi::CollectionFetchScope &fetchScope )
{
  d->mFetchScope = fetchScope;
}

Akonadi::CollectionFetchScope &CollectionFetchJob::fetchScope()
{
  return d->mFetchScope;
}

Akonadi::Collection::List CollectionFetchJob::collections() const
{
  return d->mCollections;
}

bool CollectionFetchJob::accept( Visitor *visitor )
{
  return visitor->visit( this );
}

void CollectionFetchJob::handleCollectionsReceived( const Akonadi::Collection::List &collections )
{
  d->mCollections << collections;

  emit collectionsReceived( collections );
}

#include "collectionfetchjob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
