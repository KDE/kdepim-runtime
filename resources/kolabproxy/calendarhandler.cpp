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

#include "calendarhandler.h"

#include <KCalCore/Event>
#include <KCalCore/CalFormat>

#include <kdebug.h>
#include <kmime/kmime_codecs.h>
#include <kolab/kolabobject.h>

#include <KLocale>

#include <QBuffer>
#include <QDomDocument>


CalendarHandler::CalendarHandler( const Akonadi::Collection &imapCollection ) : IncidenceHandler( imapCollection )
{
  m_mimeType = "application/x-vnd.kolab.event";
}


CalendarHandler::~CalendarHandler()
{
}

KMime::Message::Ptr CalendarHandler::incidenceToMime( const KCalCore::Incidence::Ptr& incidence )
{
  return Kolab::KolabObjectWriter::writeEvent(incidence.dynamicCast<KCalCore::Event>(), m_formatVersion, m_calendar.timeZoneId());
}


QStringList CalendarHandler::contentMimeTypes()
{
  return QStringList() << KCalCore::Event::eventMimeType();
}

QString CalendarHandler::iconName() const
{
  return QString::fromLatin1( "view-calendar" );
}
