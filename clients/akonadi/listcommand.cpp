/*
    This file is part of Akonadi.

    Copyright (c) 2006 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "listcommand.h"

#include "out.h"

#include <akonadi/collectionlistjob.h>
#include <akonadi/collectionpathresolver.h>
#include <akonadi/itemfetchjob.h>

using namespace Akonadi;

ListCommand::ListCommand( const QString &path )
  : mPath( path )
{
  if ( mPath.isEmpty() ) mPath = "/";
}

void ListCommand::exec()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( mPath );
  if ( !resolver->exec() ) {
    err() << "Error resolving path '" << mPath << "': " << resolver->errorString() << endl;
    return;
  }
  int currentColId = resolver->collection();

  CollectionListJob* collectionJob = new CollectionListJob( Collection( currentColId ) );
  if ( !collectionJob->exec() ) {
    err() << "Error listing collection '" << mPath << "': "
      << collectionJob->errorString()
      << endl;
    return;
  } else {
    foreach( const Akonadi::Collection collection, collectionJob->collections() ) {
      out() << collection.name() << endl;
    }
  }

  ItemFetchJob* itemFetchJob = new ItemFetchJob( Collection( currentColId ) );
  if ( !itemFetchJob->exec() ) {
    err() << "Error listing items at '" << mPath << "': "
      << itemFetchJob->errorString()
      << endl;
  } else {
    foreach( Akonadi::Item item, itemFetchJob->items() ) {
      QString str;
      str = QLatin1String("Item: ") + QString::number( item.reference().id() );
      if ( !item.reference().remoteId().isEmpty() ) {
        str += QLatin1String(" [") + item.reference().remoteId() + QLatin1Char(']');
      }
      if ( !item.flags().isEmpty() ) {
        str += QLatin1String(" ( ");
        foreach( QByteArray flag, item.flags() ) {
          str += flag + QLatin1Char(' ');
        }
        str += QLatin1Char(')');
      }
      str += QLatin1String(" [") + item.mimeType() + QLatin1Char(']');
      out() << str << endl;
    }
  }
}
