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

#include <kdebug.h>
#include <kmime/kmime_codecs.h>

#include <QBuffer>
#include <QDomDocument>


JournalHandler::JournalHandler()
{
  m_mimeType = "application/x-vnd.kolab.journal";
}


JournalHandler::~JournalHandler()
{
}

Akonadi::Item::List JournalHandler::translateItems(const Akonadi::Item::List & items)
{
  kDebug() << "translateItems";
  Akonadi::Item::List newItems;
  Q_FOREACH(Akonadi::Item item, items)
  {
    MessagePtr payload = item.payload<MessagePtr>();
    KCal::Journal *j = journalFromKolab(payload);
    if (j) {
      Akonadi::Item newItem("text/calendar");
      newItem.setRemoteId(QString::number(item.id()));
      JournalPtr journal(j);
      newItem.setPayload<JournalPtr>(journal);
      newItems << newItem;
    }
  }

  return newItems;
}

KCal::Journal * JournalHandler::journalFromKolab(MessagePtr data)
{
  KMime::Content *xmlContent  = findContentByType(data, m_mimeType);
  if (xmlContent) {
    QByteArray xmlData = xmlContent->decodedContent();
//     kDebug() << "xmlData " << xmlData;

    //FIXME: read the tz
    KCal::Journal *journal = Kolab::Journal::xmlToJournal(QString::fromUtf8(xmlData), QString() );
    QDomDocument doc;
    doc.setContent(QString::fromUtf8(xmlData));
    QDomNodeList nodes = doc.elementsByTagName("inline-attachment");
    for (int i = 0; i < nodes.size(); i++ ) {
      QString name = nodes.at(i).toElement().text();
      QByteArray type;
      KMime::Content *content = findContentByName(data, name, type);
      QByteArray c = content->decodedContent().toBase64();
      KCal::Attachment *attachment = new KCal::Attachment(c.data(), QString::fromLatin1(type));
      journal->addAttachment(attachment);
      kDebug() << "ATTACHEMENT NAME" << name;
    }

    return journal;
  }
  return 0L;
}

KMime::Content* JournalHandler::findContentByName(MessagePtr data, const QString &name, QByteArray &type)
{
  KMime::Content::List list = data->contents();
  Q_FOREACH(KMime::Content *c, list)
  {
    if (c->contentType()->name() == name)
      type = QByteArray(c->contentType()->type());
      return c;
  }
  return 0L;

}

void JournalHandler::toKolabFormat(const Akonadi::Item& item, Akonadi::Item &imapItem)
{
  kDebug() << "toKolabFormat";
  JournalPtr t(item.payload<JournalPtr>());
  KCal::Journal *journal = t.get();

  imapItem.setMimeType( "message/rfc822" );

  MessagePtr message(new KMime::Message);
  QString header;
  header += "From: " + journal->organizer().fullName() + "<" + journal->organizer().email() + ">\n";
  header += "Subject: " + journal->uid() + "\n";
//   header += "Date: " + QDateTime::currentDateTime().toString(Qt::ISODate) + "\n";
  header += "User-Agent: Akonadi Kolab Proxy Resource \n";
  header += "MIME-Version: 1.0\n";
  header += "X-Kolab-Type: " + m_mimeType + "\n\n\n";
  message->setContent(header.toLatin1());

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
  content->setBody(codec->encode(Kolab::Journal::journalToXML(journal, "").toUtf8()));
  message->addContent(content);

  Q_FOREACH (KCal::Attachment *attachment, journal->attachments()) {
    header = "Content-Type: "  +attachment->mimeType() + "; name=\""  + attachment->label() + "\"\n";
    header += "Content-Transfer-Encoding: base64\n";
    header += "Content-Disposition: attachment; filename=\"" + attachment->label() + "\"";
    content = new KMime::Content();
    content->setHead(header.toLatin1());
    content->setBody(attachment->data());
    message->addContent(content);

  }

  imapItem.setPayload<MessagePtr>(message);
}

QStringList  JournalHandler::contentMimeTypes()
{
  return QStringList() << "text/calendar";
}