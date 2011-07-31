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

#include <QtCore/QMap>
#include <QtCore/QString>

class DavProtocolBase;

namespace KIO {
class DavJob;
}

class KUrl;

class QDomDocument;

/**
 * @short A factory class for handling DAV jobs.
 *
 * This class provides factory methods to create preconfigured
 * low-level DAV jobs and has access to the global DAV protocol dialect
 * objects which abstract the access to the various DAV protocol dialects.
 */
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

    /**
     * Returns a preconfigured DAV PROPFIND job.
     *
     * @param url The target url of the job.
     * @param document The query XML document.
     * @param depth The Depth: value to send in the HTTP request
     */
    KIO::DavJob* createPropFindJob( const KUrl &url, const QDomDocument &document, const QString &depth = QString( "1" ) ) const;

    /**
     * Returns a preconfigured DAV REPORT job.
     *
     * @param url The target url of the job.
     * @param document The query XML document.
     * @param depth The Depth: value to send in the HTTP request
     */
    KIO::DavJob* createReportJob( const KUrl &url, const QDomDocument &document, const QString &depth = QString( "1" ) ) const;

    /**
     * Returns a preconfigured DAV PROPPATCH job.
     *
     * @param url The target url of the job.
     * @param document The query XML document.
     */
    KIO::DavJob* createPropPatchJob( const KUrl &url, const QDomDocument &document ) const;

    /**
     * Returns the DAV protocol dialect object for the given DAV @p protocol.
     */
    const DavProtocolBase* davProtocol( DavUtils::Protocol protocol );

  private:
    /**
     * Creates a new DAV manager.
     */
    DavManager();

    /**
     * Creates a new protocol.
     */
    bool createProtocol( DavUtils::Protocol protocol );

    typedef QMap<DavUtils::Protocol, DavProtocolBase*> protocolsMap;
    protocolsMap mProtocols;
    static DavManager* mSelf;
};

#endif
