/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef MIXEDMAILDIRSTORE_H
#define MIXEDMAILDIRSTORE_H

#include "abstractlocalstore.h"

class MixedMaildirStore : public Akonadi::FileStore::AbstractLocalStore
{
    Q_OBJECT

public:
    MixedMaildirStore();

    ~MixedMaildirStore() override;

protected:
    void setTopLevelCollection(const Akonadi::Collection &collection) override;
    void processJob(Akonadi::FileStore::Job *job) override;

    void checkCollectionMove(Akonadi::FileStore::CollectionMoveJob *job, int &errorCode, QString &errorText) const override;

    void checkItemCreate(Akonadi::FileStore::ItemCreateJob *job, int &errorCode, QString &errorText) const override;

    void checkItemModify(Akonadi::FileStore::ItemModifyJob *job, int &errorCode, QString &errorText) const override;

    void checkItemFetch(Akonadi::FileStore::ItemFetchJob *job, int &errorCode, QString &errorText) const override;

private:
    class Private;
    Private *const d;
};

#endif
