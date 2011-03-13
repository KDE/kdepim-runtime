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

#ifndef DAVITEMSLISTJOB_H
#define DAVITEMSLISTJOB_H

#include "davitem.h"
#include "davutils.h"

#include <kjob.h>

#include <QtCore/QSet>

/**
 * @short A job that lists all DAV items inside a DAV collection.
 */
class DavItemsListJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new dav items list job.
     *
     * @param url The url of the DAV collection.
     * @param parent The parent object.
     */
    explicit DavItemsListJob( const DavUtils::DavUrl &url, QObject *parent = 0 );

    /**
     * Starts the job.
     */
    virtual void start();

    /**
     * Returns the list of items including identifier url and etag information.
     */
    DavItem::List items() const;

  private Q_SLOTS:
    void davJobFinished( KJob* );

  private:
    DavUtils::DavUrl mUrl;
    DavItem::List mItems;
    QSet<QString> mSeenUrls; // to prevent events duplication with some servers
    uint mSubJobCount;
    bool mSubJobSuccessful;
};

#endif
