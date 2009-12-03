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

#include "objectrequestjob.h"

#include "davmanager.h"
#include "davutils.h"
#include "objectutils.h"
#include "oxutils.h"

#include <kio/davjob.h>

#include <QtCore/QDebug>

using namespace OXA;

ObjectRequestJob::ObjectRequestJob( const Object &object, QObject *parent )
  : KJob( parent ), mObject( object )
{
}

void ObjectRequestJob::start()
{
  QDomDocument document;
  QDomElement multistatus = DAVUtils::addDavElement( document, document, QLatin1String( "multistatus" ) );
  QDomElement prop = DAVUtils::addDavElement( document, multistatus, QLatin1String( "prop" ) );
  DAVUtils::addOxElement( document, prop, QLatin1String( "object_id" ), OXUtils::writeNumber( mObject.objectId() ) );

  const QString path = ObjectUtils::davPath( mObject );

  KIO::DavJob *job = DavManager::self()->createFindJob( path, document );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( davJobFinished( KJob* ) ) );
}

Object ObjectRequestJob::object() const
{
  return mObject;
}

void ObjectRequestJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

  const QDomDocument &document = davJob->response();
  qDebug() << document.toString();

  QDomElement multistatus = document.documentElement();
  QDomElement response = multistatus.firstChildElement( QLatin1String( "response" ) );
  const QDomNodeList props = response.elementsByTagName( "prop" );
  const QDomElement prop = props.at( 0 ).toElement();
  mObject = ObjectUtils::parseObject( prop, mObject.type() );

  emitResult();
}

#include "objectrequestjob.moc"
