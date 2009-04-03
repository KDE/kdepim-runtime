/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "collectiontest.h"
#include "test.h"

#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/private/collectionpathresolver_p.h>

#include <QStringList>

using namespace Akonadi;

CollectionTest::CollectionTest(QObject* parent) :
  QObject( parent )
{
}

void CollectionTest::setParent(const Akonadi::Collection& parent)
{
  mParent = parent;
}

void CollectionTest::setCollection(const Akonadi::Collection& collection)
{
  mCollection = collection;
}

void CollectionTest::setParent(const QString& parentPath)
{
  CollectionPathResolver* resolver = new CollectionPathResolver( parentPath, this );
  if ( !resolver->exec() )
    Test::instance()->fail( resolver->errorString() );
  setParent( Collection( resolver->collection() ) );
}

void CollectionTest::setCollection(const QString& path)
{
  CollectionPathResolver* resolver = new CollectionPathResolver( path, this );
  if ( !resolver->exec() )
    Test::instance()->fail( resolver->errorString() );
  setCollection( Collection( resolver->collection() ) );
}

void CollectionTest::setName(const QString& name)
{
  mCollection.setName( name );
}

void CollectionTest::addContentType(const QString& type)
{
  mCollection.setContentMimeTypes( mCollection.contentMimeTypes() << type );
}

void CollectionTest::create()
{
  mCollection.setParent( mParent );
  CollectionCreateJob* job = new CollectionCreateJob( mCollection, this );
  if ( !job->exec() )
    Test::instance()->fail( job->errorString() );
}

void CollectionTest::update()
{
  CollectionModifyJob* job = new CollectionModifyJob( mCollection, this );
  if ( !job->exec() )
    Test::instance()->fail( job->errorString() );
}

void CollectionTest::remove()
{
  CollectionDeleteJob* job = new CollectionDeleteJob( mCollection, this );
  if ( !job->exec() )
    Test::instance()->fail( job->errorString() );
}

#include "collectiontest.moc"
