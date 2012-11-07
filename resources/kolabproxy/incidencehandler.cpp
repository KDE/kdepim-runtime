/*
  Copyright (C) 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Copyright (c) 2009 Andras Mantia <andras@kdab.net>
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

#include "incidencehandler.h"

#include <libkdepim-copy/kincidencechooser.h> //krazy:exclude=camelcase

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>

#include <KCalCore/CalFormat>

#include <KLocale>

IncidenceHandler::IncidenceHandler( const Akonadi::Collection &imapCollection )
  : KolabHandler( imapCollection ),
    m_calendar( QString::fromLatin1( "UTC" ) )
{
}

IncidenceHandler::~IncidenceHandler()
{
}

Akonadi::Item::List IncidenceHandler::translateItems( const Akonadi::Item::List &items )
{
  kDebug() << "translateItems" << items.size();
  Akonadi::Item::List newItems;
  Q_FOREACH ( const Akonadi::Item &item, items ) {
    if ( !item.hasPayload<KMime::Message::Ptr>() ) {
      kWarning() << "Payload is not a MessagePtr!";
      continue;
    }
    const KMime::Message::Ptr payload = item.payload<KMime::Message::Ptr>();

    KCalCore::Incidence::Ptr incidencePtr = Kolab::KolabObjectReader(payload).getIncidence();
    if ( checkForErrors( item.id() ) ) {
      continue;
    }
    if ( !incidencePtr ) {
      kWarning() << "Failed to read incidence.";
      continue;
    }
    //TODO conflict resolving should happen for all object types,
    //and not only for incidences (move to kolabhandler)
    if ( m_uidMap.contains( incidencePtr->uid() ) ) {
      StoredItem storedItem = m_uidMap[incidencePtr->uid()];
      kDebug() << "Conflict detected for incidence uid  " << incidencePtr->uid()
               << " for imap item id = " << item.id()
               << " and the other imap item id is "
               << storedItem.id << "; imap collection is "
               << m_imapCollection.name()
               << m_imapCollection.id()
               << "; collection has rights "
               << m_imapCollection.rights();

      const Akonadi::Collection::Rights requiredRights =
        Akonadi::Collection::CanDeleteItem |
        Akonadi::Collection::CanCreateItem;

      if ( ( m_imapCollection.rights() & requiredRights ) != requiredRights ) {
        kDebug() << "Skipping conflict resolution, no rights on collection "
                 << m_imapCollection.name();
        continue;
      }

      ConflictResolution res = resolveConflict( incidencePtr );
      kDebug() << "ConflictResolution " << res;
      if ( res == Local ) {
        m_uidMap.remove( incidencePtr->uid() );
        incidencePtr = KCalCore::Incidence::Ptr();
        emit deleteItemFromImap( item );
        continue;
      } else if ( res == Remote ) {
        Akonadi::Item it( m_uidMap[incidencePtr->uid()].id );
        emit deleteItemFromImap( it );
      } else if ( res == Both ) {
        incidencePtr->setSummary( i18n( "Copy of: %1", incidencePtr->summary() ) );
        incidencePtr->setUid( KCalCore::CalFormat::createUniqueId() );
        Akonadi::Item copiedItem( incidencePtr->mimeType() );
        incidenceToItem( incidencePtr, copiedItem );
        if ( checkForErrors( item.id() ) ) {
            copiedItem.setPayloadFromData( "" );
            //TODO clear mimetype?
            continue;
        }
        Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item );
        job->fetchScope().fetchFullPayload();
        if ( job->exec() ) {
          emit addItemToImap( copiedItem, job->items()[0].storageCollectionId() );
        }
        // we can't do newItem.setRemoteId(..) yet because we don't have copiedItem.id()
        // it's being added to imap, so we continue.
        continue;
      }
    }
    m_uidMap[incidencePtr->uid()] = StoredItem( item.id(), incidencePtr );
    kDebug() << "Add to uidMap: " << incidencePtr->uid() << item.id() << incidencePtr;
    Akonadi::Item newItem( incidencePtr->mimeType() );
    newItem.setPayload( incidencePtr );
    newItem.setRemoteId( QString::number( item.id() ) );
    newItems << newItem;
  }

  return newItems;
}

IncidenceHandler::ConflictResolution IncidenceHandler::resolveConflict(
  const KCalCore::Incidence::Ptr &inc )
{
  /*
  if ( !isResolveConflictSet() ) {
        // we should do no conflict resolution
  delete inc;
  return;
}
  */
  const QString origUid = inc->uid();
  KCalCore::Incidence::Ptr localIncidence = m_uidMap[origUid].incidence;
  KCalCore::Incidence::Ptr addedIncidence = inc;
  KCalCore::Incidence::Ptr result;
  if ( localIncidence ) {
    kDebug() << "Compare  " << localIncidence << addedIncidence;
    if ( *localIncidence.data() == *addedIncidence.data() ) {
      // real duplicate, remove the second one
      return Duplicate;
    } else {
      KPIM::KIncidenceChooser *ch = new KPIM::KIncidenceChooser();
      ch->setIncidence( localIncidence, addedIncidence );
      if ( KPIM::KIncidenceChooser::chooseMode == KPIM::KIncidenceChooser::ask ) {
        connect ( this, SIGNAL(useGlobalMode()), ch, SLOT (useGlobalMode()) );
        if ( ch->exec() ) {
          if ( KPIM::KIncidenceChooser::chooseMode != KPIM::KIncidenceChooser::ask ) {
            emit useGlobalMode() ;
          }
        }
      }
      result = ch->getIncidence();
      delete ch;
    }
  } else {
    // nothing there locally, just take the new one. Can't Happen (TM)
    result = addedIncidence;
  }
  if ( result == localIncidence ) {
    //take local
    return Local;
  } else if ( result == addedIncidence ) {
    //take remote (inc)
    return Remote;
  } else if ( result == 0 ) {
    // take both
    return Both;
  }
  return Duplicate;
}

bool IncidenceHandler::toKolabFormat( const Akonadi::Item &item, Akonadi::Item &imapItem )
{
//   kDebug() << "toKolabFormat";
  KCalCore::Incidence::Ptr incidencePtr;
  if ( item.hasPayload<KCalCore::Incidence::Ptr>() ) {
    incidencePtr = item.payload<KCalCore::Incidence::Ptr>();
  } else {
    kWarning() << "item is not an incidence";
    return false;
  }
//   kDebug() << "item payload: " << item.payloadData();
  incidenceToItem( incidencePtr, imapItem );
  if ( checkForErrors( item.id() ) ) {
    imapItem.setPayloadFromData( "" );
    return false;
  }
  return true;
}

void IncidenceHandler::incidenceToItem( const KCalCore::Incidence::Ptr &incidencePtr,
                                        Akonadi::Item &imapItem )
{
  if ( !incidencePtr ) {
    imapItem.setPayloadFromData( "" );
    return;
  }
  const KMime::Message::Ptr &message = incidenceToMime( incidencePtr );
  imapItem.setMimeType( "message/rfc822" );
  imapItem.setPayload( message );
}

void IncidenceHandler::itemDeleted( const Akonadi::Item &item )
{
  QMutableMapIterator<QString, StoredItem> it( m_uidMap );
  while ( it.hasNext() ) {
    it.next();
    if ( it.value().id == item.id() ) {
      kDebug() << "Remove from uidMap: " << it.key() << item.id() << it.value().incidence;
      it.remove();
      break;
    }
  }
}

void IncidenceHandler::reset()
{
  m_uidMap.clear();
}

void IncidenceHandler::itemAdded( const Akonadi::Item &item )
{
  if ( !item.hasPayload<KMime::Message::Ptr>() ) {
    kWarning() << "Payload is not a MessagePtr!";
    return;
  }
  KMime::Message::Ptr payload = item.payload<KMime::Message::Ptr>();
  Kolab::KolabObjectReader reader;
  reader.parseMimeMessage( payload );
  if ( checkForErrors( item.id() ) ) {
    return;
  }
  KCalCore::Incidence::Ptr e = reader.getIncidence();
  if ( !e ) {
    return;
  }
  const KCalCore::Incidence::Ptr incidence( e );
  m_uidMap[e->uid()] = StoredItem( item.id(), incidence );
  kDebug() << "Add to uidMap: " << incidence->uid() << item.id() << incidence;
}
