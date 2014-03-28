/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef AKONADI_GIDMIGRATIONJOB_H
#define AKONADI_GIDMIGRATIONJOB_H

#include <akonadi/item.h>
#include <akonadi/collection.h>
#include <akonadi/job.h>
#include <QStringList>
#include <QQueue>

/**
 * @short Job that updates the gid of all items in the store.
 *
 * Requires a serializer plugin supporting the gidextractor interface for the mimetype of the objects to migrate.
 *
 * @author Christian Mollekopf <mollekopf@kolabsys.com>
 * @since 4.12
 */
class GidMigrationJob : public Akonadi::Job
{
    Q_OBJECT
public:
    /**
     * @param mimeTypeFilter The list of mimetypes of objects to be migrated.
     * @param parent The parent object.
     */
    explicit GidMigrationJob(const QStringList &mimeTypeFilter, QObject *parent = 0);

    /**
     * Destroys the item fetch job.
     */
    virtual ~GidMigrationJob();

    virtual void doStart();

private slots:
    void collectionsReceived(const Akonadi::Collection::List &);
    void collectionsFetched(KJob*);
    void itemsUpdated(KJob*);

private:
    void processCollection();
    QStringList mMimeTypeFilter;
    Akonadi::Collection::List mCollections;
};

/**
 * @internal
 */
class UpdateJob: public Akonadi::Job
{
    Q_OBJECT
public:
    explicit UpdateJob(const Akonadi::Collection &col, QObject *parent = 0);
    virtual ~UpdateJob();

    virtual void doStart();
    virtual void slotResult(KJob *job);

private slots:
    void itemsReceived(const Akonadi::Item::List &items);
private:
    bool processNext();

    const Akonadi::Collection mCollection;
    QQueue<Akonadi::Item> mItemQueue;
    bool mModJobRunning;
};

#endif
