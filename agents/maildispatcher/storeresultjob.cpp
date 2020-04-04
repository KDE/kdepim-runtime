/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "storeresultjob.h"

#include <Item>
#include <ItemFetchJob>
#include <ItemModifyJob>
#include <Akonadi/KMime/MessageFlags>
#include "maildispatcher_debug.h"
#include <KLocalizedString>
#include <MailTransportAkonadi/ErrorAttribute>
#include <MailTransportAkonadi/DispatchModeAttribute>

using namespace Akonadi;
using namespace MailTransport;

StoreResultJob::StoreResultJob(const Item &item, bool success, const QString &message, QObject *parent)
    : TransactionSequence(parent)
{
    mItem = item;
    mSuccess = success;
    mMessage = message;
}

StoreResultJob::~StoreResultJob()
{
}

void StoreResultJob::doStart()
{
    // Fetch item in case it was modified elsewhere.
    ItemFetchJob *job = new ItemFetchJob(mItem, this);
    connect(job, &ItemFetchJob::result, this, &StoreResultJob::fetchDone);
}

bool StoreResultJob::success() const
{
    return mSuccess;
}

QString StoreResultJob::message() const
{
    return mMessage;
}

void StoreResultJob::fetchDone(KJob *job)
{
    if (job->error()) {
        return;
    }

    qCDebug(MAILDISPATCHER_LOG);

    const ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetchJob);
    if (fetchJob->items().count() != 1) {
        qCCritical(MAILDISPATCHER_LOG) << "Fetched" << fetchJob->items().count() << "items, expected 1.";
        setError(Unknown);
        setErrorText(i18n("Failed to fetch item."));
        commit();
        return;
    }

    // Store result in item.
    Item item = fetchJob->items().at(0);
    if (mSuccess) {
        item.clearFlag(Akonadi::MessageFlags::Queued);
        item.setFlag(Akonadi::MessageFlags::Sent);
        item.setFlag(Akonadi::MessageFlags::Seen);
        item.removeAttribute<ErrorAttribute>();
    } else {
        item.setFlag(Akonadi::MessageFlags::HasError);
        ErrorAttribute *errorAttribute = new ErrorAttribute(mMessage);
        item.addAttribute(errorAttribute);

        // If dispatch failed, set the DispatchModeAttribute to Manual.
        // Otherwise, the user will *never* be able to try sending the mail again,
        // as Send Queued Messages will ignore it.
        if (item.hasAttribute<DispatchModeAttribute>()) {
            item.attribute<DispatchModeAttribute>()->setDispatchMode(MailTransport::DispatchModeAttribute::Manual);
        } else {
            item.addAttribute(new DispatchModeAttribute(MailTransport::DispatchModeAttribute::Manual));
        }
    }

    ItemModifyJob *modifyJob = new ItemModifyJob(item, this);
    QObject::connect(modifyJob, &ItemModifyJob::result, this, &StoreResultJob::modifyDone);
}

void StoreResultJob::modifyDone(KJob *job)
{
    if (job->error()) {
        return;
    }

    qCDebug(MAILDISPATCHER_LOG);

    commit();
}
