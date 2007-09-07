/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#include "storecommand.h"
#include "out.h"

#include <libakonadi/itemstorejob.h>
#include <libakonadi/itemfetchjob.h>

using namespace Akonadi;

StoreCommand::StoreCommand(const QString & uid,  const QString & part, const QString & content) :
  mUid( uid ), mPart( part ), mContent( content )
{
}

void StoreCommand::exec()
{
  DataReference ref( mUid.toInt(), QString() );
  ItemFetchJob* fetchJob = new ItemFetchJob( ref );
  fetchJob->fetchAllParts();
  if ( !fetchJob->exec() ) {
    err() << "Error fetching item '" << mUid << "': "
        << fetchJob->errorString()
        << endl;
  } else {
    Item item = fetchJob->items()[0];
    item.addPart( mPart, mContent.toLatin1() );
    ItemStoreJob* sJob = new ItemStoreJob( item );
    sJob->storePayload();
    if ( !sJob->exec() ) {
      err() << "Unable to store part " << mPart << " = " << mContent << " for item " << mUid <<":"
            << sJob->errorString()
            << endl;
    }
    else
      out() << "Part " << mPart << "=" << mContent << " stored for item " << mUid << endl;
  }
}
