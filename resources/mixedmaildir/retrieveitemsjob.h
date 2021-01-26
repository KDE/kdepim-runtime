/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2011 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef MIXEDMAILDIR_RETRIEVEITEMSJOB_H
#define MIXEDMAILDIR_RETRIEVEITEMSJOB_H

#include <AkonadiCore/Item>
#include <AkonadiCore/Job>

namespace Akonadi
{
class Collection;
}

class MixedMaildirStore;

/**
 * Used to implement ResourceBase::retrieveItems() for MixedMail Resource.
 * This completely bypasses ItemSync in order to achieve maximum performance.
 */
class RetrieveItemsJob : public Akonadi::Job
{
    Q_OBJECT
public:
    RetrieveItemsJob(const Akonadi::Collection &collection, MixedMaildirStore *store, QObject *parent = nullptr);

    ~RetrieveItemsJob() override;

    Akonadi::Collection collection() const;

    Akonadi::Item::List availableItems() const;

    Akonadi::Item::List itemsMarkedAsDeleted() const;

protected:
    void doStart() override;

private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void processNewItem())
    Q_PRIVATE_SLOT(d, void processChangedItem())
};

#endif
