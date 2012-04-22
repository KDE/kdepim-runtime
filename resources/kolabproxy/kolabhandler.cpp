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

#include <kolab/errorhandler.h>
#include <kpassivepopup.h>
#include <klocalizedstring.h>


KolabHandler::KolabHandler( const Akonadi::Collection &imapCollection ) : m_imapCollection( imapCollection ), m_formatVersion(Kolab::KolabV3)
{
}

KolabHandler::Ptr KolabHandler::createHandler( const QByteArray& type,
                                           const Akonadi::Collection &imapCollection )
{
  if (type ==  "contact.default" || type ==  "contact") {
    return Ptr(new AddressBookHandler( imapCollection ));
  } else if (type ==  "event.default" || type ==  "event") {
    return Ptr(new CalendarHandler( imapCollection ));
  } else if (type ==  "task.default" || type ==  "task") {
    return Ptr(new TasksHandler( imapCollection ));
  } else if (type ==  "journal.default" || type ==  "journal") {
    return Ptr(new JournalHandler( imapCollection ));
  } else if (type ==  "note.default" || type ==  "note") {
    return Ptr(new NotesHandler( imapCollection ));
  }
  return KolabHandler::Ptr();
}

KolabHandler::Ptr KolabHandler::createHandler(const KolabV2::FolderType& type, const Akonadi::Collection& imapCollection)
{
    switch (type) {
        case KolabV2::Contact:
            return Ptr(new AddressBookHandler( imapCollection ));
        case KolabV2::Event:
            return Ptr(new CalendarHandler( imapCollection ));
        case KolabV2::Task:
            return Ptr(new TasksHandler( imapCollection ));
        case KolabV2::Journal:
            return Ptr(new JournalHandler( imapCollection ));
        case KolabV2::Note:
            return Ptr(new NotesHandler( imapCollection ));
        default:
            qWarning() << "invalid type";
    }
    return KolabHandler::Ptr();
}


void KolabHandler::setKolabFormatVersion(Kolab::Version version)
{
    m_formatVersion = version;
}



QByteArray KolabHandler::kolabTypeForMimeType(const QStringList& contentMimeTypes)
{
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

bool KolabHandler::checkForErrors(Akonadi::Item::Id affectedItem)
{
    if (Kolab::ErrorHandler::instance().error() > Kolab::ErrorHandler::NoError) {
        QString errorMsg = QString("Error on item %1:\n").arg(affectedItem);
        foreach(const Kolab::ErrorHandler::Err &error, Kolab::ErrorHandler::instance().getErrors()) {
            kDebug() << "error on item " << affectedItem <<": " << error.message;
            errorMsg.append(error.message);
            errorMsg.append("\n");
        }
        KPassivePopup::message(errorMsg, (QWidget*)0 ); //Temporary for testing
    }
    
    switch (Kolab::ErrorHandler::instance().error()) {
        case Kolab::ErrorHandler::Warning:
        case Kolab::ErrorHandler::NoError:
            Kolab::ErrorHandler::instance().clear();
            return false;
        default:
            kWarning() << "error";
    }
    foreach(const Kolab::ErrorHandler::Err &error, Kolab::ErrorHandler::instance().getErrors()) {
        if (error.severity > Kolab::ErrorHandler::Warning) {
            KPassivePopup::message(
                i18n( "An error occured while reading/writing a Kolab-Groupware-Object: (%1) ", error.message )+i18n( "The following akonadi item is affected: %1", QString::number(affectedItem) ),
                (QWidget*)0 );
        }
    }
    Kolab::ErrorHandler::instance().clear();
    return true;
}

