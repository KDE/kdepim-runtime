/*
    Copyright (c) 2012 Frank Osterfeld <osterfeld@kde.org>

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

#include "jobs.h"

Job::Job( QObject* parent )
    : KJob( parent )
{
}

void Job::start()
{
    QMetaObject::invokeMethod( this, "doStart", Qt::QueuedConnection );
}

void Job::setUrl( const KUrl& url )
{
    m_url = url;
}

void Job::setPassword( const QString& password )
{
    m_password = password;
}

KUrl Job::url() const
{
    return m_url;
}

QString Job::password() const
{
    return m_password;
}

ListSubscriptionsJob::ListSubscriptionsJob( QObject* parent )
    : Job( parent )
{
}

void ListSubscriptionsJob::doStart()
{
}

#include "jobs.moc"
