/*  This file is part of the KDE project
    Copyright (c) 2007 Will Stephenson <wstephenson@kde.org>

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

#include "appendcommand.h"
#include "out.h"

#include <akonadi/collectionpathresolver.h>
#include <akonadi/itemappendjob.h>

using namespace Akonadi;

AppendCommand::AppendCommand(const QString & path, const QString & mimetype, const QString & content) : mPath( path ), mMimeType( mimetype ), mContent( content )
{
}

void AppendCommand::exec()
{
  CollectionPathResolver *resolver = new CollectionPathResolver( mPath );
  if ( !resolver->exec() ) {
    err() << "Error resolving path '" << mPath << "': " << resolver->errorString() << endl;
    return;
  }

  Collection::Id currentColId = resolver->collection();

  Item item;
  item.setMimeType( mMimeType );
  item.addPart( Item::PartBody, mContent.toUtf8() );

  ItemAppendJob* appendJob = new ItemAppendJob( item, Collection( currentColId ) );
  if ( !appendJob->exec() ) {
    err() << "Error appending item '" << endl;
  }
}
