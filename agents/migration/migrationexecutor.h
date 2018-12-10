/*
 * Copyright 2013  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef MIGRATIONEXECUTOR_H
#define MIGRATIONEXECUTOR_H

#include <KJob>
#include <QQueue>
#include <QSharedPointer>
#include <migration/migratorbase.h>

/**
 * An executor can contain multiple jobs that are scheduled by the executor.
 *
 * The executor is responsible for starting/pausing/stopping the individual migrators.
 *
 * This job is used to give overall progress information and start/stop controls to KUIServer via KUiServerJobTracker.
 */
class MigrationExecutor : public KJob
{
    Q_OBJECT
public:
    MigrationExecutor();
    void add(const QSharedPointer<MigratorBase> &);
    void start() override;

protected:
    bool doResume() override;
    bool doSuspend() override;

private Q_SLOTS:
    void onStoppedProcessing();
    void executeNext();

private:
    QQueue< QWeakPointer<MigratorBase> > mQueue;
    QWeakPointer<MigratorBase> mCurrentMigrator;
    bool mSuspended = false;
    int mTotalAmount = 0;
    int mAlreadyProcessed = 0;
};

#endif
