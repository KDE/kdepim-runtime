/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "davcollectionmodifyjob.h"
#include "davmanager.h"
#include "davutils.h"

#include <kio/davjob.h>
#include <klocale.h>
#include <kdebug.h>

DavCollectionModifyJob::DavCollectionModifyJob( const DavUtils::DavUrl &url, QObject *parent )
  : KJob( parent ), mUrl( url )
{
}

void DavCollectionModifyJob::setProperty( const QString &prop, const QString &value, const QString &ns )
{
  QDomElement propElement;

  if ( ns.isEmpty() )
    propElement = mQuery.createElement( prop );
  else
    propElement = mQuery.createElementNS( prop, ns );

  const QDomText textElement = mQuery.createTextNode( value );
  propElement.appendChild( textElement );

  mSetProperties << propElement;
}

void DavCollectionModifyJob::removeProperty( const QString &prop, const QString &ns )
{
  QDomElement propElement;

  if ( ns.isEmpty() )
    propElement = mQuery.createElement( prop );
  else
    propElement = mQuery.createElementNS( prop, ns );

  mRemoveProperties << propElement;
}

void DavCollectionModifyJob::start()
{
  if ( mSetProperties.isEmpty() && mRemoveProperties.isEmpty() ) {
    setError( UserDefinedError ); // no special meaning, for now at least
    setErrorText( i18n( "No properties to change or remove" ) );
    emitResult();
    return;
  }

  QDomDocument mQuery;
  QDomElement propertyUpdateElement = mQuery.createElementNS( "DAV:", "propertyupdate" );
  mQuery.appendChild( propertyUpdateElement );

  if ( !mSetProperties.isEmpty() ) {
    QDomElement setElement = mQuery.createElementNS( "DAV:", "set" );
    propertyUpdateElement.appendChild( setElement );

    QDomElement propElement = mQuery.createElementNS( "DAV:", "prop" );
    setElement.appendChild( propElement );

    foreach ( const QDomElement &element, mSetProperties )
      propElement.appendChild( element );
  }

  if ( !mRemoveProperties.isEmpty() ) {
    QDomElement removeElement = mQuery.createElementNS( "DAV:", "remove" );
    propertyUpdateElement.appendChild( removeElement );

    QDomElement propElement = mQuery.createElementNS( "DAV:", "prop" );
    removeElement.appendChild( propElement );

    foreach ( const QDomElement &element, mSetProperties )
      propElement.appendChild( element );
  }

  KIO::DavJob *job = DavManager::self()->createPropPatchJob( mUrl.url(), mQuery );
  job->addMetaData( "PropagateHttpHeader", "true" );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( davJobFinished( KJob* ) ) );
}

void DavCollectionModifyJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

  const int responseCode = davJob->queryMetaData( "responsecode" ).toInt();

  // Consider that a 200 status means success and proceed no further
  if ( responseCode == 200 ) {
    emitResult();
    return;
  } else if ( responseCode > 499 && responseCode < 600 ) {
    // Server-side error, unrecoverable
    setError( UserDefinedError );
    setErrorText( i18n( "The server encountered an error that prevented it from completing your request" ) );
    emitResult();
    return;
  } else if ( responseCode == 423 ) {
    // The remote resource has been locked
    setError( UserDefinedError );
    setErrorText( i18n( "The remote item has been locked, try again later" ) );
    emitResult();
    return;
  } else if ( responseCode > 399 && responseCode < 500 ) {
    // User-side error
    setError( UserDefinedError );
    setErrorText( i18n( "There was a problem with the request. The item has not been modified on the server : error %1.", responseCode ) );
    emitResult();
    return;
  }

  const QDomDocument response = davJob->response();
  QDomElement responseElement = DavUtils::firstChildElementNS( response.documentElement(), "DAV:", "response" );

  bool hasError = false;
  QString errorText;

  // parse all propstats answers to get the eventual errors
  const QDomNodeList propstats = responseElement.elementsByTagNameNS( "DAV:", "propstat" );
  for ( uint i = 0; i < propstats.length(); ++i ) {
    const QDomElement propstatElement = propstats.item( i ).toElement();
    const QDomElement statusElement = DavUtils::firstChildElementNS( propstatElement, "DAV:", "status" );

    const QString statusText = statusElement.text();
    if ( statusText.contains( "200" ) ) {
      // Nothing special to do here, this indicates the success of the whole request
      break;
    } else {
      // Generic error
      hasError = true;
      errorText = i18n( "There was an error when modifying the properties" );
    }
  }

  if ( hasError ) {
    // Trying to get more information about the error
    const QDomElement responseDescriptionElement = DavUtils::firstChildElementNS( responseElement, "DAV:", "responsedescription" );
    if ( !responseDescriptionElement.isNull() ) {
      errorText.append( i18n("\nThe server returned more information:\n") );
      errorText.append( responseDescriptionElement.text() );
    }

    setError( UserDefinedError );
    setErrorText( errorText );
  }

  emitResult();
}
