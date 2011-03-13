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

#ifndef DAVCOLLECTIONSFETCHJOB_H
#define DAVCOLLECTIONSFETCHJOB_H

#include "davcollection.h"
#include "davutils.h"

#include <kjob.h>

/**
 * @short A job that fetches all DAV collection.
 *
 * This job is used to fetch all DAV collection that are available
 * under a certain DAV url.
 */
class DavCollectionsFetchJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new dav collections fetch job.
     *
     * @param url The DAV url of the DAV collection whose sub collections shall be fetched.
     * @param parent The parent object.
     */
    explicit DavCollectionsFetchJob( const DavUtils::DavUrl &url, QObject *parent = 0 );

    /**
     * Starts the job.
     */
    virtual void start();

    /**
     * Returns the list of fetched DAV collections.
     */
    DavCollection::List collections() const;

  Q_SIGNALS:
    /**
     * This signal is emitted every time a new collection has been discovered.
     *
     * @param collectionUrl The URL of the discovered collection
     * @param configuredUrl The URL given to the job
     */
    void collectionDiscovered( int protocol, const QString &collectionUrl, const QString &configuredUrl );

  private Q_SLOTS:
    void principalFetchFinished( KJob* );
    void collectionsFetchFinished( KJob* );

  private:
    void doCollectionsFetch( const KUrl &url );

    DavUtils::DavUrl mUrl;
    DavCollection::List mCollections;
    uint mSubJobCount;
    bool mSubJobSuccessful;
};

#endif
