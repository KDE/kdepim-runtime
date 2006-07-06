/*
    This file is part of libakonadi.

    Copyright (c) 2006 Till Adam <adam@kde.org>

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

#include "resourcebase.h"
#include "resourceadaptor.h"

using namespace PIM;

class ResourceBase::Private
{
  public:
    org::kde::Akonadi::Tracer *mTracer;
    QString mId;
};

ResourceBase::ResourceBase( const QString & id )
  : d( new Private )
{
  new ResourceAdaptor( this );
  if ( !QDBus::sessionBus().registerObject( id, this, QDBusConnection::ExportAdaptors ) ) {
    qDebug( "Unable to connect to dbus service: %s", qPrintable( QDBus::sessionBus().lastError().message() ) );
  }

  d->mTracer = new org::kde::Akonadi::Tracer( "org.kde.Akonadi", "/tracing", QDBus::sessionBus(), this );
  d->mId = id;
}

ResourceBase::~ResourceBase()
{
  delete d;
}

void ResourceBase::warning( const QString& message )
{
    d->mTracer->warning( QString( "ResourceBase(%1)" ).arg( d->mId ), message );
}

void ResourceBase::error( const QString& message )
{
    d->mTracer->error( QString( "ResourceBase(%1)" ).arg( d->mId ), message );
}
