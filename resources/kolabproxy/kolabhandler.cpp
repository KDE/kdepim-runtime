/*
    Copyright (c) 2009 Andras Mantia <amantia@kde.org>

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
#include "kolabhandler.h"
#include "addressbookhandler.h"
#include "calendarhandler.h"
#include "taskshandler.h"
#include "journalhandler.h"
#include "notehandler.h"

#include <KCalCore/Event>
#include <KCalCore/Todo>
#include <KCalCore/Journal>

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>


KolabHandler::KolabHandler( const Akonadi::Collection &imapCollection ) : m_imapCollection( imapCollection )
{
}

KolabHandler *KolabHandler::createHandler( const QByteArray& type,
                                           const Akonadi::Collection &imapCollection )
{
  if (type ==  "contact.default" || type ==  "contact") {
    return new AddressBookHandler( imapCollection );
  } else if (type ==  "event.default" || type ==  "event") {
    return new CalendarHandler( imapCollection );
  } else if (type ==  "task.default" || type ==  "task") {
    return new TasksHandler( imapCollection );
  } else if (type ==  "journal.default" || type ==  "journal") {
    return new JournalHandler( imapCollection );
  } else if (type ==  "note.default" || type ==  "note") {
    return new NotesHandler( imapCollection );
  } else {
    return 0;
  }
}


QByteArray KolabHandler::kolabTypeForCollection(const Akonadi::Collection& collection)
{
  const QStringList contentMimeTypes = collection.contentMimeTypes();
  if ( contentMimeTypes.contains( KABC::Addressee::mimeType() ) ) {
    return "contact";
  } else if ( contentMimeTypes.contains( KCalCore::Event::eventMimeType() ) ) {
    return "event";
  } else if ( contentMimeTypes.contains( KCalCore::Todo::todoMimeType() ) ) {
    return "task";
  } else if ( contentMimeTypes.contains( KCalCore::Journal::journalMimeType() ) ) {
    return "journal";
  } else if ( contentMimeTypes.contains( "application/x-vnd.akonadi.note" ) || contentMimeTypes.contains( "text/x-vnd.akonadi.note" ) ) {
    return "note";
  }
  return QByteArray();
}

QStringList KolabHandler::allSupportedMimeTypes()
{
  return QStringList()
   << KABC::Addressee::mimeType()
   << KABC::ContactGroup::mimeType()
   << KCalCore::Event::eventMimeType()
   << KCalCore::Todo::todoMimeType()
   << KCalCore::Journal::journalMimeType()
   << QLatin1String( "application/x-vnd.akonadi.note" )
   << QLatin1String( "text/x-vnd.akonadi.note" );
}


KolabHandler::~KolabHandler()
{
}

QByteArray KolabHandler::mimeType() const
{
  return m_mimeType;
}
