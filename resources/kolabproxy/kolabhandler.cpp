/*
  Copyright (c) 2009 Andras Mantia <amantia@kde.org>
  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>

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
#include "notehandler.h"
#include "taskshandler.h"
#include "imapitemaddedjob.h"
#include "imapitemremovedjob.h"

#include <errorhandler.h> //libkolab

#include <KLocalizedString>
#include <QQueue>

KolabHandler::KolabHandler( const Akonadi::Collection &imapCollection )
  : m_imapCollection( imapCollection ),
    m_formatVersion ( Kolab::KolabV3 ),
    m_warningDisplayLevel( Kolab::ErrorHandler::Error ),
    mItemAddJobInProgress( false )
{
}

KolabHandler::Ptr KolabHandler::createHandler( Kolab::FolderType type,
                                               const Akonadi::Collection &imapCollection )
{
  switch (type) {
  case Kolab::ContactType:
    return Ptr( new AddressBookHandler( imapCollection ) );
  case Kolab::EventType:
    return Ptr( new CalendarHandler( imapCollection ) );
  case Kolab::TaskType:
    return Ptr( new TasksHandler( imapCollection ) );
  case Kolab::JournalType:
    return Ptr( new JournalHandler( imapCollection ) );
  case Kolab::NoteType:
    return Ptr( new NotesHandler( imapCollection ) );
  default:
    qWarning() << "invalid type";
  }
  return KolabHandler::Ptr();
}

bool KolabHandler::hasHandler( Kolab::FolderType type )
{
  switch (type) {
  case Kolab::ContactType:
  case Kolab::EventType:
  case Kolab::TaskType:
  case Kolab::JournalType:
  case Kolab::NoteType:
    return true;
  default:
    return false;
  }
  return false;
}

KolabHandler::Ptr KolabHandler::createHandler( const KolabV2::FolderType &type,
                                               const Akonadi::Collection &imapCollection )
{
  switch (type) {
  case KolabV2::Contact:
    return Ptr( new AddressBookHandler( imapCollection ) );
  case KolabV2::Event:
    return Ptr( new CalendarHandler( imapCollection ) );
  case KolabV2::Task:
    return Ptr( new TasksHandler( imapCollection ) );
  case KolabV2::Journal:
    return Ptr( new JournalHandler( imapCollection ) );
  case KolabV2::Note:
    return Ptr( new NotesHandler( imapCollection ) );
  default:
    qWarning() << "invalid type";
  }
  return KolabHandler::Ptr();
}

void KolabHandler::setKolabFormatVersion( Kolab::Version version )
{
  m_formatVersion = version;
}

QByteArray KolabHandler::kolabTypeForMimeType( const QStringList &contentMimeTypes )
{
  if ( contentMimeTypes.contains( KABC::Addressee::mimeType() ) ) {
    return "contact";
  } else if ( contentMimeTypes.contains( KCalCore::Event::eventMimeType() ) ) {
    return "event";
  } else if ( contentMimeTypes.contains( KCalCore::Todo::todoMimeType() ) ) {
    return "task";
  } else if ( contentMimeTypes.contains( KCalCore::Journal::journalMimeType() ) ) {
    return "journal";
  } else if ( contentMimeTypes.contains( QLatin1String("application/x-vnd.akonadi.note") ) ||
              contentMimeTypes.contains( QLatin1String("text/x-vnd.akonadi.note") ) ) {
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

bool KolabHandler::checkForErrors( Akonadi::Item::Id affectedItem )
{
  if ( Kolab::ErrorHandler::instance().error() < m_warningDisplayLevel ) {
    Kolab::ErrorHandler::instance().clear();
    return false;
  }

  QString errorMsg;
  foreach ( const Kolab::ErrorHandler::Err &error, Kolab::ErrorHandler::instance().getErrors() ) {
    errorMsg.append( error.message );
    errorMsg.append( QLatin1String("\n") );
  }

  kWarning() << "Error on item " << affectedItem << ":\n" << errorMsg;
  Kolab::ErrorHandler::instance().clear();
  return true;
}

Akonadi::Item::List KolabHandler::resolveConflicts(const Akonadi::Item::List& kolabItems)
{
  //we should preserve the order here
  Akonadi::Item::List finalItems;
  QMap<QString, Akonadi::Item::List> gidItemMap;
  foreach (const Akonadi::Item &item, kolabItems) {
      const QString gid = extractGid(item);
      if (!gid.isEmpty()) {
        gidItemMap[gid] << item;
      }
  }
  foreach (const Akonadi::Item &item, kolabItems) {
      const QString gid = extractGid(item);
      if (gid.isEmpty()) {
        finalItems << item;
      } else if (gidItemMap.contains(gid)) {
        //TODO assuming the items are in revers imap uid order (newest first)
        finalItems << gidItemMap.value(gid).first();
        gidItemMap.remove(gid);
      }
  }
  return finalItems;
}

void KolabHandler::processItemAddedQueue()
{
  if (mItemAddedQueue.isEmpty() || mItemAddJobInProgress) {
    return;
  }
  //TODO we would only have to serialize add jobs for items with the same GID
  mItemAddJobInProgress = true;
  const QPair<Akonadi::Item, Akonadi::Collection> pair = mItemAddedQueue.dequeue();
  ImapItemAddedJob *addedJob = new ImapItemAddedJob( pair.first, pair.second, *this, this );
  connect(addedJob, SIGNAL(result(KJob*)), this, SLOT(onItemAdded(KJob*)));
  addedJob->start();
}

void KolabHandler::onItemAdded(KJob *job)
{
  mItemAddJobInProgress = false;
  if (job->error()) {
    kWarning() << job->errorString();
  }
  processItemAddedQueue();
}

void KolabHandler::imapItemAdded(const Akonadi::Item& imapItem, const Akonadi::Collection& imapCollection)
{
  mItemAddedQueue.enqueue(qMakePair<Akonadi::Item, Akonadi::Collection>(imapItem, imapCollection));
  processItemAddedQueue();
}

void KolabHandler::imapItemRemoved(const Akonadi::Item& imapItem)
{
  //TODO delay this in case an imapItemAdded job is already running (it might reuse the item)
  ImapItemRemovedJob *job = new ImapItemRemovedJob(imapItem, this);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)));
  job->start();
}

void KolabHandler::checkResult(KJob* job)
{
  if ( job->error() ) {
    kWarning() << "Error occurred: " << job->errorString();
  }
}

