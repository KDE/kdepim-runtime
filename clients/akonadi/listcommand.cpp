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

#include <libakonadi/collectionlistjob.h>

ListCommand::ListCommand( const QString &path )
  : mPath( path )
{
  if ( mPath.isEmpty() ) mPath = "/";
}

void ListCommand::exec()
{
  PIM::CollectionListJob job( mPath.toUtf8() );
  if ( !job.exec() ) {
    err() << "Error listing /: " << job.errorText() << endl;
  } else {
    foreach( PIM::Collection *collection, job.collections() ) {
      out() << collection->name() << endl;
    }
  }
}
