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

#include "itemtest.h"

#include "global.h"
#include "test.h"

#include <akonadi/private/collectionpathresolver_p.h>
#include <akonadi/itemcreatejob.h>

#include <KDebug>
#include <QFile>

using namespace Akonadi;

ItemTest::ItemTest( QObject *parent ) :
  QObject( parent )
{
}

void ItemTest::setParentCollection(const Akonadi::Collection& parent)
{
  mParent = parent;
}

void ItemTest::setParentCollection(const QString& path)
{
  CollectionPathResolver* resolver = new CollectionPathResolver( path, this );
  if ( !resolver->exec() )
    Test::instance()->fail( resolver->errorString() );
  setParentCollection( Collection( resolver->collection() ) );
}

void ItemTest::setMimeType(const QString& mimeType)
{
  mItem.setMimeType( mimeType );
}

void ItemTest::setPayloadFromFile(const QString& fileName)
{
  QFile file( Global::basePath() + fileName );
  if ( !file.open( QFile::ReadOnly ) )
    Test::instance()->fail( file.errorString() );
  mItem.setPayloadFromData( file.readAll() );
}

void ItemTest::create()
{
  ItemCreateJob* job = new ItemCreateJob( mItem, mParent, this );
  if ( !job->exec() )
    Test::instance()->fail( job->errorString() );
  mItem = job->item();
}

#include "itemtest.moc"
