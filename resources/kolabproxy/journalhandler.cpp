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
#include <kolabformatV2/journal.h>

#include <KCalCore/Journal>

#include <kdebug.h>
#include <kmime/kmime_codecs.h>

#include <QBuffer>
#include <QDomDocument>

JournalHandler::JournalHandler( const Akonadi::Collection &imapCollection) : IncidenceHandler( imapCollection )
{
  m_mimeType = "application/x-vnd.kolab.journal";
}


JournalHandler::~JournalHandler()
{
}

KCalCore::Incidence::Ptr JournalHandler::incidenceFromKolab(const KMime::Message::Ptr &data)
{
  return incidenceFromKolabImpl<KCalCore::Journal::Ptr, KolabV2::Journal>( data );
}

QByteArray JournalHandler::incidenceToXml( const KCalCore::Incidence::Ptr &incidence)
{
  return KolabV2::Journal::journalToXML( incidence.dynamicCast<KCalCore::Journal>(), m_calendar.timeZoneId()).toUtf8();
}

QStringList  JournalHandler::contentMimeTypes()
{
  return QStringList() << KCalCore::Journal::journalMimeType();
}

QString JournalHandler::iconName() const
{
  return QString::fromLatin1( "view-pim-journal" );
}
