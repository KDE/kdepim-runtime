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

#include "taskshandler.h"
#include "task.h"

#include <kdebug.h>
#include <kmime/kmime_codecs.h>

#include <QBuffer>
#include <QDomDocument>


TasksHandler::TasksHandler( const Akonadi::Collection &imapCollection ) : IncidenceHandler( imapCollection )
{
  m_mimeType = "application/x-vnd.kolab.task";
}


TasksHandler::~TasksHandler()
{
}

KCalCore::Incidence::Ptr TasksHandler::incidenceFromKolab( const KMime::Message::Ptr &data )
{
  return incidenceFromKolabImpl<KCalCore::Todo::Ptr, Kolab::Task>( data );
}

QByteArray TasksHandler::incidenceToXml( const KCalCore::Incidence::Ptr &incidence )
{
  return Kolab::Task::taskToXML( incidence.dynamicCast<KCalCore::Todo>(), m_calendar.timeZoneId() ).toUtf8();
}

QStringList TasksHandler::contentMimeTypes()
{
  return QStringList() << KCalCore::Todo::todoMimeType();
}

QString TasksHandler::iconName() const
{
  return QString::fromLatin1( "view-pim-tasks" );
}
