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
#include <libakonadi/itemfetchjob.h>
#include <kmime/kmime_message.h>

using namespace Akonadi;

ListCommand::ListCommand( const QString &path )
  : mPath( path )
{
  if ( mPath.isEmpty() ) mPath = "/";
}

void ListCommand::exec()
{
  CollectionListJob* collectionJob = new CollectionListJob( mPath.toUtf8() );
  if ( !collectionJob->exec() ) {
    err() << "Error listing collection '" << mPath << "': "
      << collectionJob->errorString()
      << endl;
    return;
  } else {
    foreach( Akonadi::Collection *collection, collectionJob->collections() ) {
      out() << collection->name() << endl;
    }
  }

  ItemFetchJob* itemFetchJob = new ItemFetchJob( mPath.toUtf8() );
  if ( !itemFetchJob->exec() ) {
    err() << "Error listing items at '" << mPath << "': "
      << itemFetchJob->errorString()
      << endl;
  } else {
    foreach( Akonadi::Item *item, itemFetchJob->items() ) {
      QString str;
      str = QLatin1String("Item: ") + QString::number( item->reference().persistanceID() );
      if ( !item->reference().externalUrl().isEmpty() ) {
        str += QLatin1String(" [") + item->reference().externalUrl().toString() + QLatin1Char(']');
      }
      if ( !item->flags().isEmpty() ) {
        str += QLatin1String(" ( ");
        foreach( QByteArray flag, item->flags() ) {
          str += flag + QLatin1Char(' ');
        }
        str += QLatin1Char(')');
      }
      str += QLatin1String(" [") + item->mimeType() + QLatin1Char(']');
      out() << str << endl;
    }
  }

#if 0
  Akonadi::MessageFetchJob messageJob( mPath.toUtf8() );
  if ( !messageJob.exec() ) {
    err() << "Error listing messages at '" << mPath << "': "
      << messageJob.errorString()
      << endl;
  } else {
    foreach( Akonadi::Message *m, messageJob.messages() ) {
      KMime::Message *message = m->mime();
      out() << "Subject: " << message->subject()->asUnicodeString() << endl;
    }
  }
#endif
}
