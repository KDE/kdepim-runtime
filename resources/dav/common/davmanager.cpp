/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "davmanager.h"

#include "caldavprotocol.h"
#include "carddavprotocol.h"
#include "groupdavprotocol.h"

#include <kio/davjob.h>
#include <KDebug>

#include <QtXml/QDomDocument>

DavManager* DavManager::mSelf = 0;

DavManager::DavManager()
{
}

DavManager::~DavManager()
{
  QMapIterator<DavUtils::Protocol, DavProtocolBase*> it( mProtocols );
  while ( it.hasNext() ) {
    it.next();
    delete it.value();
  }
}

DavManager* DavManager::self()
{
  if ( !mSelf )
    mSelf = new DavManager();

  return mSelf;
}

KIO::DavJob* DavManager::createPropFindJob( const KUrl &url, const QDomDocument &document ) const
{
  const QString davDepth( '1' );
  KIO::DavJob *job = KIO::davPropFind( url, document, davDepth, KIO::HideProgressInfo | KIO::DefaultFlags );

  // workaround needed, Depth: header doesn't seem to be correctly added
  const QString header = "Content-Type: text/xml\r\nDepth: " + davDepth;
  job->addMetaData( "customHTTPHeader", header );
  job->addMetaData( "cookies", "none" );
  job->addMetaData( "no-auth-prompt", "true" );
  job->setProperty( "extraDavDepth", QVariant::fromValue( davDepth ) );

  return job;
}

KIO::DavJob* DavManager::createReportJob( const KUrl &url, const QDomDocument &document ) const
{
  const QString davDepth( '1' );
  KIO::DavJob *job = KIO::davReport( url, document.toString(), davDepth, KIO::HideProgressInfo | KIO::DefaultFlags );

  // workaround needed, Depth: header doesn't seem to be correctly added
  const QString header = "Content-Type: text/xml\r\nDepth: " + davDepth;
  job->addMetaData( "customHTTPHeader", header );
  job->addMetaData( "cookies", "none" );
  job->addMetaData( "no-auth-prompt", "true" );
  job->setProperty( "extraDavDepth", QVariant::fromValue( davDepth ) );

  return job;
}

KIO::DavJob* DavManager::createPropPatchJob( const KUrl &url, const QDomDocument &document ) const
{
  KIO::DavJob *job = KIO::davPropPatch( url, document, KIO::HideProgressInfo | KIO::DefaultFlags );
  const QString header = "Content-Type: text/xml";
  job->addMetaData( "customHTTPHeader", header );
  job->addMetaData( "cookies", "none" );
  job->addMetaData( "no-auth-prompt", "true" );
  return job;
}

const DavProtocolBase* DavManager::davProtocol( DavUtils::Protocol protocol )
{
  if ( createProtocol( protocol ) )
    return mProtocols[ protocol ];
  else
    return 0;
}

bool DavManager::createProtocol( DavUtils::Protocol protocol )
{
  if ( mProtocols.contains( protocol ) )
    return true;

  switch( protocol ) {
    case DavUtils::CalDav:
      mProtocols.insert( DavUtils::CalDav, new CaldavProtocol() );
      break;
    case DavUtils::CardDav:
      mProtocols.insert( DavUtils::CardDav, new CarddavProtocol() );
      break;
    case DavUtils::GroupDav:
      mProtocols.insert( DavUtils::GroupDav, new GroupdavProtocol() );
      break;
    default:
      kError() << "Unknown protocol: " << static_cast<int>( protocol );
      return false;
  }

  return true;
}
