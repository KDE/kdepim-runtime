/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>
    Based on DavItemsListJob which is copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#ifndef DAVITEMSFETCHJOB_H
#define DAVITEMSFETCHJOB_H

#include "davitem.h"
#include "davutils.h"

#include <kjob.h>

#include <QtCore/QMap>
#include <QtCore/QStringList>

/**
 * @short A job that fetches a list of items from a DAV server using a multiget query.
 */
class DavItemsFetchJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new items fetch job.
     *
     * @param collectionUrl The DAV collection on which to run the query
     * @param urls The list of urls to fetch
     * @param parent The parent object
     */
    DavItemsFetchJob( const DavUtils::DavUrl &collectionUrl, const QStringList &urls, QObject *parent = 0 );

    /**
     * Starts the job.
     */
    virtual void start();

    /**
     * Returns the list of fetched items
     */
    DavItem::List items() const;

    /**
     * Return the item found at @p url
     */
    DavItem item( const QString &url ) const;

  private Q_SLOTS:
    void davJobFinished( KJob* );

  private:
    DavUtils::DavUrl mCollectionUrl;
    QStringList mUrls;
    QMap<QString, DavItem> mItems;
};

#endif
