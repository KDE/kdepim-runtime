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

#include <akonadi/itemmodifyjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

using namespace Akonadi;

StoreCommand::StoreCommand(const QString & uid, const QString & content) :
  mUid( uid ), mContent( content )
{
}

void StoreCommand::exec()
{
  const Item item( mUid.toLongLong() );
  ItemFetchJob* fetchJob = new ItemFetchJob( item );
  if ( !fetchJob->exec() ) {
    err() << "Error fetching item '" << mUid << "': "
        << fetchJob->errorString()
        << endl;
  } else {
    Item item = fetchJob->items()[0];
    item.setPayloadFromData( mContent.toUtf8() );
    ItemModifyJob* sJob = new ItemModifyJob( item );
    if ( !sJob->exec() ) {
      err() << "Unable to store data " << mContent << " for item " << mUid <<":"
            << sJob->errorString()
            << endl;
    }
    else
      out() << "Data stored for item " << mUid << endl;
  }
}
