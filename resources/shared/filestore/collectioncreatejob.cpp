/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

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

#include "collectioncreatejob.h"

#include "session_p.h"

using namespace Akonadi;

using namespace Akonadi::FileStore;

class CollectionCreateJob::Private
{
  public:
    explicit Private( CollectionCreateJob *parent )
      : mParent( parent )
    {
    }

  public:
    Collection mCollection;
    Collection mTargetParent;

  private:
    CollectionCreateJob *mParent;
};

CollectionCreateJob::CollectionCreateJob( const Collection &collection, const Collection &targetParent, AbstractJobSession *session )
  : Job( session ), d( new Private( this ) )
{
  Q_ASSERT( session != 0 );

  d->mCollection = collection;
  d->mTargetParent = targetParent;

  session->addJob( this );
}

CollectionCreateJob::~CollectionCreateJob()
{
  delete d;
}

Akonadi::Collection CollectionCreateJob::collection() const
{
  return d->mCollection;
}

Akonadi::Collection CollectionCreateJob::targetParent() const
{
  return d->mTargetParent;
}

bool CollectionCreateJob::accept( Visitor *visitor )
{
  return visitor->visit( this );
}

void CollectionCreateJob::handleCollectionCreated( const Collection &collection )
{
  d->mCollection = collection;
}

#include "collectioncreatejob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
