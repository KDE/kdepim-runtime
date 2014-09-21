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

#include "itemaddedjob.h"
#include <Akonadi/ItemCreateJob>
#include <Akonadi/KMime/MessageFlags>
#include <klocale.h>

ItemAddedJob::ItemAddedJob(const Akonadi::Item& kolabItem, const Akonadi::Collection& col, KolabHandler& handler, QObject* parent)
    :KJob(parent),
    mHandler(handler),
    mKolabItem(kolabItem),
    mParentCollection(col)
{

}

void ItemAddedJob::start()
{
    QMetaObject::invokeMethod(this, "doStart", Qt::QueuedConnection);
}

void ItemAddedJob::doStart()
{
    const Akonadi::Collection imapCollection = kolabToImap(mParentCollection);
    kDebug() << imapCollection.id();
    Akonadi::Item imapItem( "message/rfc822" );
    if (!mHandler.toKolabFormat(mKolabItem, imapItem)) {
        kWarning() << "Failed to convert item to kolab format: " << mKolabItem.id();
        setError(KJob::UserDefinedError);
        setErrorText(i18n("Failed to convert item %1 to kolab format", mKolabItem.id()));
        emitResult();
        return;
    }
    //FIXME remove
    imapItem.setFlag(Akonadi::MessageFlags::Seen);

    Akonadi::ItemCreateJob *cjob = new Akonadi::ItemCreateJob(imapItem, imapCollection);
    connect(cjob, SIGNAL(result(KJob*)), SLOT(onItemCreatedDone(KJob*)));
}

void ItemAddedJob::onItemCreatedDone(KJob* job)
{
    if (job->error()) {
        setError(KJob::UserDefinedError);
    } else {
        Akonadi::ItemCreateJob *cjob = static_cast<Akonadi::ItemCreateJob*>(job);
        mImapItem = cjob->item();
    }
    emitResult();
}

Akonadi::Item ItemAddedJob::kolabItem() const
{
    return mKolabItem;
}

Akonadi::Item ItemAddedJob::imapItem() const
{
    return mImapItem;
}
