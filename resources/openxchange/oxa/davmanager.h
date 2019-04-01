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

#ifndef OXA_DAVMANAGER_H
#define OXA_DAVMANAGER_H

#include <QUrl>

namespace KIO {
class DavJob;
}

class QDomDocument;

namespace OXA {
/**
 * @short A class that manages DAV specific information.
 *
 * The DavManager stores the base url of the DAV service that
 * shall be accessed and provides factory methods for creating
 * DAV find and patch jobs.
 *
 * @author Tobias Koenig <tokoe@kde.org>
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
    static DavManager *self();

    /**
     * Sets the base @p url the DAV manager should use.
     */
    void setBaseUrl(const QUrl &url);

    /**
     * Returns the base url the DAV manager uses.
     */
    QUrl baseUrl() const;

    /**
     * Returns a new DAV find job.
     *
     * @param path The path that is appended to the base url.
     * @param document The request XML document.
     */
    KIO::DavJob *createFindJob(const QString &path, const QDomDocument &document) const;

    /**
     * Returns a new DAV patch job.
     *
     * @param path The path that is appended to the base url.
     * @param document The request XML document.
     */
    KIO::DavJob *createPatchJob(const QString &path, const QDomDocument &document) const;

private:
    /**
     * Creates a new DAV manager.
     */
    DavManager();

    QUrl mBaseUrl;
    static DavManager *mSelf;
};
}

#endif
