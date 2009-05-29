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
#include <libkdepim/kincidencechooser.h>
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
  Q_FOREACH(Akonadi::Item item, items)
  {
    if (!item.hasPayload<MessagePtr>()) {
      kWarning() << "Payload is not a MessagePtr!";
      continue;
    }
    MessagePtr payload = item.payload<MessagePtr>();
    KCal::Incidence *inc = incidenceFromKolab(payload);
    kDebug() << "KCal::Incidence " << inc;
    if (inc) {
      IncidencePtr incidencePtr(inc);
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
          Akonadi::Item copiedItem("text/calendar");
          incidenceToItem(incidencePtr, copiedItem);
          Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item );
          job->fetchScope().fetchFullPayload();
          if (job->exec()) {
            emit addItemToImap(copiedItem, job->items()[0].collectionId());
          }
        }
      }
      m_uidMap[incidencePtr->uid()] = StoredItem(item.id(), incidencePtr);
      kDebug() << "Add to uidMap: " << incidencePtr->uid() << item.id() << incidencePtr;
      Akonadi::Item newItem("text/calendar");
      newItem.setPayload(incidencePtr);
      newItem.setRemoteId(QString::number(item.id()));
      newItems << newItem;
    }
  }

  return newItems;
}

IncidenceHandler::ConflictResolution IncidenceHandler::resolveConflict( IncidencePtr inc)
{
  /*
  if ( ! isResolveConflictSet() ) {
        // we should do no conflict resolution
  delete inc;
  return;
}
  */
  const QString origUid = inc->uid();
  KCal::Incidence* local = m_uidMap[origUid].incidence.get();
  KCal::Incidence* localIncidence = 0;
  KCal::Incidence* addedIncidence = 0;
  KCal::Incidence*  result = 0;
  if ( local ) {
    KCal::ComparisonVisitor visitor;
    kDebug() << "Compare  " << local << inc.get();
    if ( visitor.compare( local, inc.get() ) ) {
        // real duplicate, remove the second one
      return Duplicate;
    } else {
      KPIM::KIncidenceChooser* ch = new KPIM::KIncidenceChooser();
      ch->setIncidence( local ,inc.get() );
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
    result = inc.get();
  }
  if ( result == local ) { //take local
    return Local;
  } else  if ( result == inc.get() ) { //take remote (inc)
    return Remote;
  } else if ( result == 0 ) { // take both
    return Both;
  }
  return Duplicate;
}

void IncidenceHandler::toKolabFormat(const Akonadi::Item& item, Akonadi::Item &imapItem)
{
  kDebug() << "toKolabFormat";
  IncidencePtr incidencePtr;
  if (item.hasPayload<IncidencePtr>()) {
    incidencePtr = item.payload<IncidencePtr>();
  }
  kDebug() << "item payload: " << item.payloadData();
  incidenceToItem(incidencePtr, imapItem);
}

void IncidenceHandler::incidenceToItem(IncidencePtr incidencePtr, Akonadi::Item &imapItem)
{
  if (!incidencePtr.get())
  {
    imapItem.setPayloadFromData("");
    return;
  }
  imapItem.setMimeType( "message/rfc822" );

  MessagePtr message(new KMime::Message);
  QString header;
  header += "From: " + incidencePtr->organizer().fullName() + "<" + incidencePtr->organizer().email() + ">\n";
  header += "Subject: " + incidencePtr->uid() + "\n";
  header += "Date: " + QDateTime::currentDateTime().toString(Qt::TextDate) + "\n";
  header += "User-Agent: Akonadi Kolab Proxy Resource \n";
  header += "MIME-Version: 1.0\n";
  header += "X-Kolab-Type: " + m_mimeType + "\n\n\n";
  message->setHead(header.toLatin1());

  KMime::Content *content = new KMime::Content();
  QByteArray contentData = QByteArray("Content-Type: text/plain; charset=\"us-ascii\"\nContent-Transfer-Encoding: 7bit\n\n") +
      "This is a Kolab Groupware object.\n" +
      "To view this object you will need an email client that can understand the Kolab Groupware format.\n" +
      "For a list of such email clients please visit\n"
      "http://www.kolab.org/kolab2-clients.html\n";
  content->setContent(contentData);
  message->addContent(content);

  content = new KMime::Content();
  header = "Content-Type: " + m_mimeType + "; name=\"kolab.xml\"\n";
  header += "Content-Transfer-Encoding: quoted-printable\n";
  header += "Content-Disposition: attachment; filename=\"kolab.xml\"";
  content->setHead(header.toLatin1());
  KMime::Codec *codec = KMime::Codec::codecForName( "quoted-printable" );
  content->setBody(codec->encode(incidenceToXml(incidencePtr.get())));
  message->addContent(content);

  Q_FOREACH (KCal::Attachment *attachment, incidencePtr->attachments()) {
    header = "Content-Type: "  +attachment->mimeType() + "; name=\""  + attachment->label() + "\"\n";
    header += "Content-Transfer-Encoding: base64\n";
    header += "Content-Disposition: attachment; filename=\"" + attachment->label() + "\"";
    content = new KMime::Content();
    content->setHead(header.toLatin1());
    content->setBody(attachment->data());
    message->addContent(content);

  }

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
  if (!item.hasPayload<MessagePtr>()) {
    kWarning() << "Payload is not a MessagePtr!";
    return;
  }
  MessagePtr payload = item.payload<MessagePtr>();
  KCal::Incidence *e = incidenceFromKolab(payload);
  IncidencePtr incidence(e);
  m_uidMap[e->uid()] = StoredItem(item.id(), incidence);
  kDebug() << "Add to uidMap: " << incidence->uid() << item.id() << incidence;
}

