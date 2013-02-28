/*
 *   Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "nepomukcleanerjob.h"

#include <nepomuk2/datamanagement.h>
#include <KUrl>

NepomukCleanerJob::NepomukCleanerJob(const QList<Akonadi::Item::Id> &staleItems, QObject* parent)
    : KJob(parent),
    mStaleItems(staleItems),
    mBatchSize(10)
{

}

void NepomukCleanerJob::start()
{
    remove();
}

void NepomukCleanerJob::remove()
{
    if (mStaleItems.isEmpty()) {
        emitResult();
        return;
    }
    QList <QUrl> items;
    for (int i = 0; !mStaleItems.isEmpty() && i < mBatchSize; i++) {
        const Akonadi::Item::Id id = mStaleItems.takeLast();
        kDebug() << "removing item " << id;
        items << Akonadi::Item(id).url();
    }
    KJob *removeJob = Nepomuk2::removeResources( items, Nepomuk2::RemoveSubResoures );
    connect(removeJob, SIGNAL(finished(KJob*)), this, SLOT(removedItem(KJob*)));
    removeJob->start();
}

void NepomukCleanerJob::removedItem(KJob *job)
{
    job->deleteLater();
    remove();
}

#include "nepomukcleanerjob.moc"

