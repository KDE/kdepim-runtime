/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "objectdeletejob.h"

#include "davmanager.h"
#include "davutils.h"
#include "objectutils.h"
#include "oxutils.h"

#include <kio/davjob.h>

using namespace OXA;

ObjectDeleteJob::ObjectDeleteJob( const Object &object, QObject *parent )
  : KJob( parent ), mObject( object )
{
}

void ObjectDeleteJob::start()
{
  QDomDocument document;
  QDomElement propertyupdate = DAVUtils::addDavElement( document, document, QLatin1String( "propertyupdate" ) );
  QDomElement set = DAVUtils::addDavElement( document, propertyupdate, QLatin1String( "set" ) );
  QDomElement prop = DAVUtils::addDavElement( document, set, QLatin1String( "prop" ) );
  DAVUtils::addOxElement( document, prop, QLatin1String( "object_id" ), OXUtils::writeNumber( mObject.objectId() ) );
  DAVUtils::addOxElement( document, prop, QLatin1String( "folder_id" ), OXUtils::writeNumber( mObject.folderId() ) );
  DAVUtils::addOxElement( document, prop, QLatin1String( "method" ), OXUtils::writeString( QLatin1String( "DELETE" ) ) );
  DAVUtils::addOxElement( document, prop, QLatin1String( "last_modified" ), OXUtils::writeString( mObject.lastModified() ) );

  const QString path = ObjectUtils::davPath( mObject.module() );

  KIO::DavJob *job = DavManager::self()->createPatchJob( path, document );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( davJobFinished( KJob* ) ) );
}

void ObjectDeleteJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

  const QDomDocument document = davJob->response();

  QString errorText;
  if ( DAVUtils::davErrorOccurred( document, errorText ) ) {
    setError( UserDefinedError );
    setErrorText( errorText );
    emitResult();
    return;
  }

  emitResult();
}

#include "objectdeletejob.moc"
