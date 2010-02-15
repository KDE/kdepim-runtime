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

#ifndef DAVMANAGER_H
#define DAVMANAGER_H

#include "davutils.h"

#include <QtCore/QString>

class DavProtocolBase;

namespace KIO {
class DavJob;
}

class KUrl;

class QDomDocument;

class DavManager
{
  public:
    /**
     * Destroys the DAV manager.
     */
    ~DavManager();

    /**
     * Returns the global instance of the DAV manager.
     */
    static DavManager* self();

    void setProtocol( DavUtils::Protocol protocol );
    DavUtils::Protocol protocol() const;

    void setUser( const QString &user );
    QString user() const;

    void setPassword( const QString &password );
    QString password() const;

    KIO::DavJob* createPropFindJob( const KUrl &url, const QDomDocument &document ) const;

    KIO::DavJob* createReportJob( const KUrl &url, const QDomDocument &document ) const;

    const DavProtocolBase* davProtocol() const;

  private:
    /**
     * Creates a new DAV manager.
     */
    DavManager();

    DavUtils::Protocol mProtocol;
    QString mUser;
    QString mPassword;
    DavProtocolBase *mDavProtocol;
    static DavManager* mSelf;
};

#endif
