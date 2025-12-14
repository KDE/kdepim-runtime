/*
 * SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#pragma once

#include "migration/migratorbase.h"
#include <KJob>
#include <QQueue>
#include <QSharedPointer>

/**
 * An executor can contain multiple jobs that are scheduled by the executor.
 *
 * The executor is responsible for starting/pausing/stopping the individual migrators.
 */
class MigrationExecutor : public KJob
{
    Q_OBJECT
public:
    MigrationExecutor();
    void add(const QSharedPointer<MigratorBase> &);
    void start() override;

private:
    void onStoppedProcessing();
    void executeNext();
    QQueue<QWeakPointer<MigratorBase>> mQueue;
    QWeakPointer<MigratorBase> mCurrentMigrator;
    int mTotalAmount = 0;
    int mAlreadyProcessed = 0;
};
