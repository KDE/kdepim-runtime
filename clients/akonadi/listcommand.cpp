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
#include <libakonadi/messagefetchjob.h>
#include <libkmime/kmime_message.h>

ListCommand::ListCommand( const QString &path )
  : mPath( path )
{
  if ( mPath.isEmpty() ) mPath = "/";
}

void ListCommand::exec()
{
  PIM::CollectionListJob collectionJob( mPath.toUtf8() );
  if ( !collectionJob.exec() ) {
    err() << "Error listing collection '" << mPath << "': "
      << collectionJob.errorText()
      << endl;
  } else {
    foreach( PIM::Collection *collection, collectionJob.collections() ) {
      out() << collection->name() << endl;
    }
  }

  PIM::MessageFetchJob messageJob( mPath.toUtf8() );
  if ( !messageJob.exec() ) {
    err() << "Error listing messages at '" << mPath << "': "
      << messageJob.errorText()
      << endl;
  } else {
    foreach( PIM::Message *m, messageJob.messages() ) {
      KMime::Message *message = m->mime();
      out() << "Subject: " << message->subject()->asUnicodeString() << endl;
    }
  }
}
