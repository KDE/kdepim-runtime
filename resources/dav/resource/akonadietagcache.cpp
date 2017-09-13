/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>

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

#include "akonadietagcache.h"

#include <collection.h>
#include <item.h>
#include <itemfetchjob.h>
#include <itemfetchscope.h>

using namespace KDAV;

AkonadiEtagCache::AkonadiEtagCache(const Akonadi::Collection &collection, QObject *parent)
    : KDAV::EtagCache(parent)
{
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(collection);
    job->fetchScope().fetchFullPayload(false);   // We only need the remote id and the revision
    connect(job, &Akonadi::ItemFetchJob::result, this, &AkonadiEtagCache::onItemFetchJobFinished);
    job->start();
}

void AkonadiEtagCache::onItemFetchJobFinished(KJob *job)
{
    if (job->error()) {
        return;
    }

    const Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    const Akonadi::Item::List items = fetchJob->items();

    for (const Akonadi::Item &item : items) {
        if (!contains(item.remoteId())) {
            setEtagInternal(item.remoteId(), item.remoteRevision());
        }
    }
}

