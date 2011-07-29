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

#include "objectmodifyjob.h"

#include "davmanager.h"
#include "davutils.h"
#include "objectutils.h"
#include "oxutils.h"

#include <kio/davjob.h>

using namespace OXA;

ObjectModifyJob::ObjectModifyJob( const Object &object, QObject *parent )
  : KJob( parent ), mObject( object )
{
}

void ObjectModifyJob::start()
{
  if ( ObjectUtils::needsPreloading( mObject ) ) {
    KJob *job = ObjectUtils::preloadJob( mObject );
    connect( job, SIGNAL(result(KJob*)), SLOT(preloadingJobFinished(KJob*)) );
    job->start();
  } else {
    QDomDocument document;
    QDomElement propertyupdate = DAVUtils::addDavElement( document, document, QLatin1String( "propertyupdate" ) );
    QDomElement set = DAVUtils::addDavElement( document, propertyupdate, QLatin1String( "set" ) );
    QDomElement prop = DAVUtils::addDavElement( document, set, QLatin1String( "prop" ) );

    ObjectUtils::addObjectElements( document, prop, mObject );

    const QString path = ObjectUtils::davPath( mObject.module() );

    KIO::DavJob *job = DavManager::self()->createPatchJob( path, document );
    connect( job, SIGNAL(result(KJob*)), SLOT(davJobFinished(KJob*)) );
  }
}

Object ObjectModifyJob::object() const
{
  return mObject;
}

void ObjectModifyJob::preloadingJobFinished( KJob *job )
{
  void *preloadedData = ObjectUtils::preloadData( mObject, job );

  QDomDocument document;
  QDomElement propertyupdate = DAVUtils::addDavElement( document, document, QLatin1String( "propertyupdate" ) );
  QDomElement set = DAVUtils::addDavElement( document, propertyupdate, QLatin1String( "set" ) );
  QDomElement prop = DAVUtils::addDavElement( document, set, QLatin1String( "prop" ) );

  ObjectUtils::addObjectElements( document, prop, mObject, preloadedData );

  const QString path = ObjectUtils::davPath( mObject.module() );

  KIO::DavJob *davJob = DavManager::self()->createPatchJob( path, document );
  connect( davJob, SIGNAL(result(KJob*)), SLOT(davJobFinished(KJob*)) );
}

void ObjectModifyJob::davJobFinished( KJob *job )
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

  QDomElement multistatus = document.documentElement();
  QDomElement response = multistatus.firstChildElement( QLatin1String( "response" ) );
  const QDomNodeList props = response.elementsByTagName( "prop" );
  const QDomElement prop = props.at( 0 ).toElement();

  QDomElement element = prop.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "last_modified" ) )
      mObject.setLastModified( OXUtils::readString( element.text() ) );

    element = element.nextSiblingElement();
  }

  emitResult();
}

#include "objectmodifyjob.moc"
