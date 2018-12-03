/*
    Copyright (c) 2016 Stefan Stäglich <sstaeglich@kdemail.net>

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

#ifndef TOMBOYITEMSDOWNLOADJOB_H
#define TOMBOYITEMSDOWNLOADJOB_H

#include "tomboyjobbase.h"
#include <AkonadiCore/Item>

class TomboyItemsDownloadJob : public TomboyJobBase
{
    Q_OBJECT
public:
    // ctor
    explicit TomboyItemsDownloadJob(Akonadi::Collection::Id id, KIO::AccessManager *manager, QObject *parent = nullptr);
    // returns the parsed results wrapped in Akonadi::Item::List, see bellow
    Akonadi::Item::List items() const;

    // automatically called by KJob
    void start() override;

private:
    Akonadi::Collection::Id mCollectionId;
    // This will be called once the request is finished.
    void onRequestFinished();
    Akonadi::Item::List mResultItems;
};

#endif // TOMBOYITEMSDOWNLOADJOB_H
