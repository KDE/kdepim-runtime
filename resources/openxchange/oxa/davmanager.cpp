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

#include "davmanager.h"

#include <kio/davjob.h>

#include <QtXml/QDomDocument>

#include <QtCore/QDebug>

using namespace OXA;

DavManager* DavManager::mSelf = 0;

DavManager::DavManager()
{
}

DavManager::~DavManager()
{
}

DavManager* DavManager::self()
{
  if ( !mSelf )
    mSelf = new DavManager();

  return mSelf;
}

void DavManager::setBaseUrl( const KUrl &url )
{
  mBaseUrl = url;
}

KUrl DavManager::baseUrl() const
{
  return mBaseUrl;
}

KIO::DavJob* DavManager::createFindJob( const QString &path, const QDomDocument &document ) const
{
  KUrl url( mBaseUrl );
  url.setPath( path );

  qDebug() << document.toString();
  return KIO::davPropFind( url, document, "0", KIO::HideProgressInfo );
}

KIO::DavJob* DavManager::createPatchJob( const QString &path, const QDomDocument &document ) const
{
  KUrl url( mBaseUrl );
  url.setPath( path );

  qDebug() << document.toString();
  return KIO::davPropPatch( url, document, KIO::HideProgressInfo );
}
