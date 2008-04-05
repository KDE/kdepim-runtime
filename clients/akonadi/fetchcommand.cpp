/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "fetchcommand.h"
#include "out.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

using namespace Akonadi;

FetchCommand::FetchCommand(const QString & uid, const QString & part ) :
    mUid( uid ),  mPart( part )
{
  if ( mPart.isEmpty() )
    mPart = Item::FullPayload;
}

void FetchCommand::exec()
{
  const Item item( mUid.toLongLong() );
  ItemFetchJob* fetchJob = new ItemFetchJob( item );
  fetchJob->fetchScope().addFetchPart( mPart );
  if ( !fetchJob->exec() ) {
    err() << "Error fetching item '" << mUid << "': "
        << fetchJob->errorString()
        << endl;
  } else {
    foreach( Item item, fetchJob->items() ) {
      QByteArray data = item.payloadData();
      out() << data << endl;
    }
  }
}
