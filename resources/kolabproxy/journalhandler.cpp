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
#include <kcal/kcalmimetypevisitor.h>


JournalHandler::JournalHandler() : IncidenceHandler()
{
  m_mimeType = "application/x-vnd.kolab.journal";
}


JournalHandler::~JournalHandler()
{
}

KCal::Incidence* JournalHandler::incidenceFromKolab(MessagePtr data)
{
  return journalFromKolab(data);
}


KCal::Journal * JournalHandler::journalFromKolab(MessagePtr data)
{
  KMime::Content *xmlContent  = findContentByType(data, m_mimeType);
  if (xmlContent) {
    QByteArray xmlData = xmlContent->decodedContent();
//     kDebug() << "xmlData " << xmlData;

    KCal::Journal *journal = Kolab::Journal::xmlToJournal(QString::fromUtf8(xmlData), m_calendar.timeZoneId() );
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

QByteArray JournalHandler::incidenceToXml(KCal::Incidence *incidence)
{
  return Kolab::Journal::journalToXML(dynamic_cast<KCal::Journal*>(incidence), m_calendar.timeZoneId()).toUtf8();
}

QStringList  JournalHandler::contentMimeTypes()
{
  return QStringList() << Akonadi::KCalMimeTypeVisitor::journalMimeType();
}
