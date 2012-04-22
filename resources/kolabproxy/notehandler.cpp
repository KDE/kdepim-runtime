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

#include "notehandler.h"

#include <akonadi/item.h>
#include <kolab/kolabobject.h>
#include <QStringList>

NotesHandler::NotesHandler( const Akonadi::Collection &imapCollection ) : JournalHandler( imapCollection )
{
  m_mimeType = "application/x-vnd.kolab.note";
}

QStringList NotesHandler::contentMimeTypes()
{
  return QStringList() << QLatin1String( "text/x-vnd.akonadi.note" );
}

void NotesHandler::toKolabFormat(const Akonadi::Item& item, Akonadi::Item& imapItem)
{
  if ( item.hasPayload<KMime::Message::Ptr>() ) {
    KMime::Message::Ptr note = item.payload<KMime::Message::Ptr>();
    KMime::Message::Ptr msg = Kolab::KolabObjectWriter::writeNote(note, m_formatVersion);
    if (checkForErrors(item.id())) {
        return;
    }
    imapItem.setMimeType( "message/rfc822" );
    imapItem.setPayload(msg);
  } else {
    kWarning() << "Payload is not a note!";
    return;
  }
}

Akonadi::Item::List NotesHandler::translateItems(const Akonadi::Item::List& kolabItems)
{
  Akonadi::Item::List newItems;
  foreach ( const Akonadi::Item &item, kolabItems ) {
    if (!item.hasPayload<KMime::Message::Ptr>()) {
      kWarning() << "Payload is not a MessagePtr!";
      continue;
    }
    const KMime::Message::Ptr payload = item.payload<KMime::Message::Ptr>();
    Akonadi::Item noteItem( "text/x-vnd.akonadi.note" );
    bool ret = noteFromKolab(payload, noteItem );
    if (checkForErrors(item.id())) {
      continue;
    }
    if ( ret ) {
      noteItem.setRemoteId( QString::number( item.id() ) );
      newItems.append( noteItem );
    } else {
      kWarning() << "Failed to convert kolab item ( id:" << item.id() << "rid:" << item.remoteId() << ") to Note message";
      continue;
    }
  }

  return newItems;
}

QString NotesHandler::iconName() const
{
  return QString::fromLatin1( "view-pim-notes" );
}

bool NotesHandler::noteFromKolab(const KMime::Message::Ptr& kolabMsg, Akonadi::Item& noteItem)
{
  Kolab::KolabObjectReader reader(kolabMsg);
  if(reader.getType() != Kolab::NoteObject) {
      return false;
  }
  noteItem.setPayload( reader.getNote() );
  return true;
}

