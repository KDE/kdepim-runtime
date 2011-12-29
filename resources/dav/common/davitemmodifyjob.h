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

#ifndef DAVITEMMODIFYJOB_H
#define DAVITEMMODIFYJOB_H

#include "davitem.h"
#include "davutils.h"

#include <kjob.h>

/**
 * @short A job that modifies a DAV item on the DAV server.
 */
class DavItemModifyJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new dav item modify job.
     *
     * @param url The url of the item that shall be modified.
     * @param item The item that shall be modified.
     * @param parent The parent object.
     */
    DavItemModifyJob( const DavUtils::DavUrl &url, const DavItem &item, QObject *parent = 0 );

    /**
     * Starts the job.
     */
    virtual void start();

    /**
     * Returns the modified item including the updated etag information.
     */
    DavItem item() const;

  private Q_SLOTS:
    void davJobFinished( KJob* );
    void itemRefreshed( KJob* );

  private:
    DavUtils::DavUrl mUrl;
    DavItem mItem;
};

#endif
