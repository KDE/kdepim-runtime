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

#include "foldercreatejob.h"

#include "davmanager.h"
#include "davutils.h"
#include "folderutils.h"
#include "oxutils.h"

#include <kio/davjob.h>

using namespace OXA;

FolderCreateJob::FolderCreateJob( const Folder &folder, QObject *parent )
  : KJob( parent ), mFolder( folder )
{
}

void FolderCreateJob::start()
{
  QDomDocument document;
  QDomElement propertyupdate = DAVUtils::addDavElement( document, document, QLatin1String( "propertyupdate" ) );
  QDomElement set = DAVUtils::addDavElement( document, propertyupdate, QLatin1String( "set" ) );
  QDomElement prop = DAVUtils::addDavElement( document, set, QLatin1String( "prop" ) );

  FolderUtils::addFolderElements( document, prop, mFolder );

  const QString path = QLatin1String( "/servlet/webdav.folders" );

  KIO::DavJob *job = DavManager::self()->createPatchJob( path, document );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( davJobFinished( KJob* ) ) );
}

Folder FolderCreateJob::folder() const
{
  return mFolder;
}

void FolderCreateJob::davJobFinished( KJob *job )
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
    if ( element.tagName() == QLatin1String( "object_id" ) )
      mFolder.setObjectId( OXUtils::readNumber( element.text() ) );
    else if ( element.tagName() == QLatin1String( "last_modified" ) )
      mFolder.setLastModified( OXUtils::readString( element.text() ) );

    element = element.nextSiblingElement();
  }

  emitResult();
}

#include "foldercreatejob.moc"
