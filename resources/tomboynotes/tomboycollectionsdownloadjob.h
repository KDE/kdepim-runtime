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

#ifndef TOMBOYCOLLECTIONSDOWNLOADJOB_H
#define TOMBOYCOLLECTIONSDOWNLOADJOB_H

#include "tomboyjobbase.h"
#include <AkonadiCore/Collection>

class TomboyCollectionsDownloadJob : public TomboyJobBase
{
    Q_OBJECT
public:
    // ctor
    explicit TomboyCollectionsDownloadJob(const QString &collectionName, KIO::AccessManager *manager, QObject *parent = Q_NULLPTR);
    // returns the parsed results wrapped in Akonadi::Collection::List, see bellow
    Akonadi::Collection::List collections() const;

    // automatically called by KJob
    void start() Q_DECL_OVERRIDE;

private Q_SLOTS:
    // This will be called once the request is finished.
    void onRequestFinished();

private:
    Akonadi::Collection::List mResultCollections;
    QString mCollectionName;
};

#endif // TOMBOYCOLLECTIONSDOWNLOADJOB_H
