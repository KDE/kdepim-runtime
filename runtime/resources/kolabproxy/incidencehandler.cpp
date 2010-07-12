/*
    Copyright (C) 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Copyright (c) 2009 Andras Mantia <andras@kdab.net>

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

#include "journalhandler.h"
#include "journal.h"

#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include <kdebug.h>
#include <kmime/kmime_codecs.h>
#include <kcal/comparisonvisitor.h>
#include <kcal/calformat.h>
#include <akonadi/kcal/incidencemimetypevisitor.h>
#include <libkdepim-copy/kincidencechooser.h>
#include <KLocale>

#include <QBuffer>
#include <QDomDocument>


IncidenceHandler::IncidenceHandler() : KolabHandler(), m_calendar( QString::fromLatin1("UTC") )
{
}


IncidenceHandler::~IncidenceHandler()
{
}


Akonadi::Item::List IncidenceHandler::translateItems(const Akonadi::Item::List & items)
{
  kDebug() << "translateItems" << items.size();
  Akonadi::Item::List newItems;
  Q_FOREACH(const Akonadi::Item &item, items)
  {
    if (!item.hasPayload<KMime::Message::Ptr>()) {
      kWarning() << "Payload is not a MessagePtr!";
      continue;
    }
    const KMime::Message::Ptr payload = item.payload<KMime::Message::Ptr>();
    KCal::Incidence *inc = incidenceFromKolab(payload);
    kDebug() << "KCal::Incidence " << inc;
    if (inc) {
      Akonadi::IncidenceMimeTypeVisitor visitor;
      inc->accept( visitor );
      KCal::Incidence::Ptr incidencePtr(inc);
      if (m_uidMap.contains(incidencePtr->uid())) {
        ConflictResolution res = resolveConflict(incidencePtr);
        kDebug() << "ConflictResolution " << res;
        if (res == Local) {
          m_uidMap.remove(incidencePtr->uid());
          incidencePtr.reset();
          emit deleteItemFromImap(item);
          continue;
        } else if (res == Remote) {
          Akonadi::Item it(m_uidMap[incidencePtr->uid()].id);
          emit deleteItemFromImap(it);
        } else if (res == Both) {
          incidencePtr->setSummary( i18n("Copy of: %1", incidencePtr->summary() ) );
          incidencePtr->setUid( KCal::CalFormat::createUniqueId() );
          Akonadi::Item copiedItem( visitor.mimeType() );
          incidenceToItem(incidencePtr, copiedItem);
          Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item );
          job->fetchScope().fetchFullPayload();
          if (job->exec()) {
            emit addItemToImap(copiedItem, job->items()[0].storageCollectionId());
          }
          // we can't do newItem.setRemoteId(..) yet because we don't have copiedItem.id()
          // it's being added to imap, so we continue.
          continue;
        }
      }
      m_uidMap[incidencePtr->uid()] = StoredItem(item.id(), incidencePtr);
      kDebug() << "Add to uidMap: " << incidencePtr->uid() << item.id() << incidencePtr;
      Akonadi::Item newItem( visitor.mimeType() );
      newItem.setPayload(incidencePtr);
      newItem.setRemoteId(QString::number(item.id()));
      newItems << newItem;
    }
  }

  return newItems;
}

IncidenceHandler::ConflictResolution IncidenceHandler::resolveConflict( const KCal::Incidence::Ptr &inc)
{
  /*
  if ( ! isResolveConflictSet() ) {
        // we should do no conflict resolution
  delete inc;
  return;
}
  */
  const QString origUid = inc->uid();
  KCal::Incidence* localIncidence = m_uidMap[origUid].incidence.get();
  KCal::Incidence* addedIncidence = inc.get();
  KCal::Incidence* result = 0;
  if ( localIncidence ) {
    KCal::ComparisonVisitor visitor;
    kDebug() << "Compare  " << localIncidence << addedIncidence;
    if ( visitor.compare( localIncidence, addedIncidence ) ) {
      // real duplicate, remove the second one
      return Duplicate;
    } else {
      KPIM::KIncidenceChooser* ch = new KPIM::KIncidenceChooser();
      ch->setIncidence( localIncidence, addedIncidence );
      if ( KPIM::KIncidenceChooser::chooseMode == KPIM::KIncidenceChooser::ask ) {
        connect ( this, SIGNAL( useGlobalMode() ), ch, SLOT (  useGlobalMode() ) );
        if ( ch->exec() )
          if ( KPIM::KIncidenceChooser::chooseMode != KPIM::KIncidenceChooser::ask )
            emit useGlobalMode() ;
      }
      result = ch->getIncidence();
      delete ch;
    }
  } else {
    // nothing there locally, just take the new one. Can't Happen (TM)
    result = addedIncidence;
  }
  if ( result == localIncidence ) { //take local
    return Local;
  } else  if ( result == addedIncidence ) { //take remote (inc)
    return Remote;
  } else if ( result == 0 ) { // take both
    return Both;
  }
  return Duplicate;
}

void IncidenceHandler::toKolabFormat(const Akonadi::Item& item, Akonadi::Item &imapItem)
{
  kDebug() << "toKolabFormat";
  KCal::Incidence::Ptr incidencePtr;
  if (item.hasPayload<KCal::Incidence::Ptr>()) {
    incidencePtr = item.payload<KCal::Incidence::Ptr>();
  }
  kDebug() << "item payload: " << item.payloadData();
  incidenceToItem(incidencePtr, imapItem);
}

void IncidenceHandler::incidenceToItem(const KCal::Incidence::Ptr &incidencePtr, Akonadi::Item &imapItem)
{
  if (!incidencePtr.get())
  {
    imapItem.setPayloadFromData("");
    return;
  }
  imapItem.setMimeType( "message/rfc822" );

  KMime::Message::Ptr message = createMessage( m_mimeType );
  message->from()->addAddress( incidencePtr->organizer().email().toUtf8(), incidencePtr->organizer().name() );
  message->subject()->fromUnicodeString( incidencePtr->uid(), "utf-8" );

  KMime::Content *content = createMainPart( m_mimeType, incidenceToXml(incidencePtr.get() ) );
  message->addContent( content );

  Q_FOREACH (KCal::Attachment *attachment, incidencePtr->attachments()) {
    content = createAttachmentPart( attachment->mimeType(), attachment->label(), attachment->decodedData() );
    message->addContent( content );
  }

  message->assemble();
  imapItem.setPayload(message);
}



void IncidenceHandler::itemDeleted(const Akonadi::Item &item)
{
  QMutableMapIterator<QString, StoredItem> it(m_uidMap);
  while (it.hasNext()) {
    it.next();
    if (it.value().id == item.id()) {
      kDebug() << "Remove from uidMap: " << it.key() << item.id() << it.value().incidence;
      it.remove();
      break;
    }
  }
}

void IncidenceHandler::itemAdded(const Akonadi::Item& item)
{
  if (!item.hasPayload<KMime::Message::Ptr>()) {
    kWarning() << "Payload is not a MessagePtr!";
    return;
  }
  KMime::Message::Ptr payload = item.payload<KMime::Message::Ptr>();
  KCal::Incidence *e = incidenceFromKolab(payload);
  if ( !e )
    return;
  const KCal::Incidence::Ptr incidence(e);
  m_uidMap[e->uid()] = StoredItem(item.id(), incidence);
  kDebug() << "Add to uidMap: " << incidence->uid() << item.id() << incidence;
}


void IncidenceHandler::attachmentsFromKolab(const KMime::Message::Ptr& data, const QByteArray& xmlData, KCal::Incidence* incidence)
{
  QDomDocument doc;
  doc.setContent(QString::fromUtf8(xmlData));
  QDomNodeList nodes = doc.elementsByTagName("inline-attachment");
  for (int i = 0; i < nodes.size(); i++ ) {
    const QString name = nodes.at(i).toElement().text();
    QByteArray type;
    KMime::Content *content = findContentByName(data, name, type);
    const QByteArray c = content->decodedContent().toBase64();
    KCal::Attachment *attachment = new KCal::Attachment(c.data(), QString::fromLatin1(type));
    attachment->setLabel( name );
    incidence->addAttachment(attachment);
    kDebug() << "ATTACHEMENT NAME" << name << type;
  }
}
