/*
    SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "migratorbase.h"
#include <Akonadi/Collection>

class KJob;
class ICalCategoriesToTagsMigrator : public MigratorBase
{
    Q_OBJECT
public:
    explicit ICalCategoriesToTagsMigrator();

    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] QString description() const override;

    [[nodiscard]] bool shouldAutostart() const override;

    void pause() override;
    void abort() override;
    void resume() override;

protected:
    void startWork() override;

    void discoverCalendarCollections();
    void migrateNextCollection();
    void migrateCollection(const Akonadi::Collection &collection);

private:
    Akonadi::Collection::List mCompletedCollections;
    Akonadi::Collection::List mPendingCollections;
    qsizetype mTotalItemsCount = 0;
    qsizetype mTotalProcessedItemsCount = 0;
    qsizetype mProcessedItemsCount = 0;
    KJob *mCurrentJob = nullptr;

    bool mAbort = false;
    bool mPause = false;
};