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

#include "caldavcalendar.h"
#include "davimplementation.h"
#include "groupdavcalendar.h"

#include <kio/davjob.h>

#include <QtXml/QDomDocument>

DavManager* DavManager::mSelf = 0;

DavManager::DavManager()
  : mProtocol( CalDav ), mDavProtocol( new CaldavCalendar )
{
}

DavManager::~DavManager()
{
  delete mDavProtocol;
}

DavManager* DavManager::self()
{
  if ( !mSelf )
    mSelf = new DavManager();

  return mSelf;
}

void DavManager::setProtocol( Protocol protocol )
{
  mProtocol = protocol;

  delete mDavProtocol;
  if ( mProtocol == CalDav )
    mDavProtocol = new CaldavCalendar;
  else
    mDavProtocol = new GroupdavCalendar;
}

DavManager::Protocol DavManager::protocol() const
{
  return mProtocol;
}

void DavManager::setUser( const QString &user )
{
  mUser = user;
}

QString DavManager::user() const
{
  return mUser;
}

void DavManager::setPassword( const QString &password )
{
  mPassword = password;
}

QString DavManager::password() const
{
  return mPassword;
}

KIO::DavJob* DavManager::createPropFindJob( const KUrl &url, const QDomDocument &document ) const
{
  KUrl fullUrl( url );
  fullUrl.setUser( mUser );
  fullUrl.setPassword( mPassword );

  const QString davDepth = "1";
  KIO::DavJob *job = KIO::davPropFind( fullUrl, document, davDepth, KIO::HideProgressInfo | KIO::DefaultFlags );

  // workaround needed, Depth: header doesn't seem to be correctly added
  const QString header = "Content-Type: text/xml\r\nDepth: " + davDepth;
  job->addMetaData( "customHTTPHeader", header );
  job->setProperty( "extraDavDepth", QVariant::fromValue( davDepth ) );

  return job;
}

KIO::DavJob* DavManager::createReportJob( const KUrl &url, const QDomDocument &document ) const
{
  KUrl fullUrl( url );
  fullUrl.setUser( mUser );
  fullUrl.setPassword( mPassword );

  const QString davDepth = "1";
  KIO::DavJob *job = KIO::davReport( fullUrl, document.toString(), davDepth, KIO::HideProgressInfo | KIO::DefaultFlags );

  // workaround needed, Depth: header doesn't seem to be correctly added
  const QString header = "Content-Type: text/xml\r\nDepth: " + davDepth;
  job->addMetaData( "customHTTPHeader", header );
  job->setProperty( "extraDavDepth", QVariant::fromValue( davDepth ) );

  return job;
}

const DavImplementation* DavManager::davProtocol() const
{
  return mDavProtocol;
}
