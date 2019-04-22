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

#include <QTest>
#include <QSignalSpy>
#include <QDebug>
#include <KJobTrackerInterface>

#include "../migrationscheduler.h"
#include <migration/migratorbase.h>

Q_DECLARE_METATYPE(QModelIndex)

class Testmigrator : public MigratorBase
{
    Q_OBJECT
public:
    explicit Testmigrator(const QString &identifier, QObject *parent = nullptr)
        : MigratorBase(QLatin1String("testmigrator") + identifier, QString(), QString(), parent)
        , mAutostart(false)
    {
    }

    QString displayName() const override
    {
        return QStringLiteral("name");
    }

    void startWork() override
    {
    }

    void abort() override
    {
        setMigrationState(Aborted);
    }

    virtual void complete()
    {
        setMigrationState(Complete);
    }

    bool shouldAutostart() const override
    {
        return mAutostart;
    }

    void pause() override
    {
        setMigrationState(Paused);
    }

    void resume() override
    {
        setMigrationState(InProgress);
    }

    bool mAutostart;
};

class TestJobTracker : public KJobTrackerInterface
{
public:
    TestJobTracker() : mPercent(0)
    {
    }

    void registerJob(KJob *job) override
    {
        KJobTrackerInterface::registerJob(job);
        mJobs << job;
    }

    void unregisterJob(KJob *job) override
    {
        mJobs.removeAll(job);
    }

    void finished(KJob *job) override
    {
        mJobs.removeAll(job);
    }

    void percent(KJob *job, long unsigned int percent) override
    {
        Q_UNUSED(job);
        mPercent = percent;
    }

    QList<KJob *> mJobs;
    int mPercent;
};

class SchedulerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void initTestcase()
    {
        qRegisterMetaType<QModelIndex>();
    }

    void testInsertRow()
    {
        MigrationScheduler scheduler;
        QAbstractItemModel &model(scheduler.model());

        QCOMPARE(model.rowCount(), 0);

        QSignalSpy rowsInsertedSpy(&model, &QAbstractItemModel::rowsInserted);
        QVERIFY(rowsInsertedSpy.isValid());

        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QStringLiteral("id"))));
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(rowsInsertedSpy.count(), 1);

        QVERIFY(model.index(0, 0).isValid());
        QVERIFY(!model.index(1, 0).isValid());

        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QStringLiteral("id2"))));
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(rowsInsertedSpy.count(), 2);
    }

    void testDisplayName()
    {
        MigrationScheduler scheduler;
        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QStringLiteral("id"))));
        QAbstractItemModel &model(scheduler.model());
        QCOMPARE(model.data(model.index(0, 0)).toString(), QStringLiteral("name"));
    }

    void testStartStop()
    {
        MigrationScheduler scheduler;
        QSharedPointer<Testmigrator> migrator(new Testmigrator(QStringLiteral("id")));
        scheduler.addMigrator(migrator);

        scheduler.start(migrator->identifier());
        QCOMPARE(migrator->migrationState(), MigratorBase::InProgress);

        scheduler.abort(migrator->identifier());
        QCOMPARE(migrator->migrationState(), MigratorBase::Aborted);
    }

    void testNoDuplicates()
    {
        MigrationScheduler scheduler;
        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QStringLiteral("id"))));
        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QStringLiteral("id"))));
        QAbstractItemModel &model(scheduler.model());
        QCOMPARE(model.rowCount(), 1);
    }

    void testMigrationStateChanged()
    {
        MigrationScheduler scheduler;
        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QStringLiteral("id1"))));
        QSharedPointer<Testmigrator> migrator(new Testmigrator(QStringLiteral("id2")));
        scheduler.addMigrator(migrator);
        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QStringLiteral("id3"))));
        QAbstractItemModel &model(scheduler.model());

        QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
        QVERIFY(spy.isValid());
        migrator->start();

        QCOMPARE(spy.count(), 1);
        const QVariantList args = spy.takeFirst();
        QCOMPARE(args.at(0).value<QModelIndex>().row(), 1);
        QCOMPARE(args.at(1).value<QModelIndex>().row(), 1);
    }

    void testRunMultiple()
    {
        MigrationScheduler scheduler;

        QSharedPointer<Testmigrator> m1(new Testmigrator(QStringLiteral("id1")));
        scheduler.addMigrator(m1);

        QSharedPointer<Testmigrator> m2(new Testmigrator(QStringLiteral("id2")));
        scheduler.addMigrator(m2);

        scheduler.start(m1->identifier());
        scheduler.start(m2->identifier());

        QCOMPARE(m1->migrationState(), MigratorBase::InProgress);
        QCOMPARE(m2->migrationState(), MigratorBase::InProgress);
    }

    void testRunAutostart()
    {
        MigrationScheduler scheduler;

        QSharedPointer<Testmigrator> m1(new Testmigrator(QStringLiteral("id1")));
        m1->mAutostart = true;
        scheduler.addMigrator(m1);

        QSharedPointer<Testmigrator> m2(new Testmigrator(QStringLiteral("id2")));
        m2->mAutostart = true;
        scheduler.addMigrator(m2);

        QCOMPARE(m1->migrationState(), MigratorBase::InProgress);
        qDebug() << m2->migrationState();
        QCOMPARE(m2->migrationState(), MigratorBase::None);
        m1->complete();
        QCOMPARE(m2->migrationState(), MigratorBase::InProgress);
    }

    void testJobTracker()
    {
        TestJobTracker jobTracker;
        MigrationScheduler scheduler(&jobTracker);
        QSharedPointer<Testmigrator> m1(new Testmigrator(QStringLiteral("id1")));
        m1->mAutostart = true;
        scheduler.addMigrator(m1);

        QCOMPARE(jobTracker.mJobs.size(), 1);

        m1->complete();

        //Give the job some time to delete itself
        QTest::qWait(500);

        QCOMPARE(jobTracker.mJobs.size(), 0);
    }

    void testSuspend()
    {
        TestJobTracker jobTracker;
        MigrationScheduler scheduler(&jobTracker);
        QSharedPointer<Testmigrator> m1(new Testmigrator(QStringLiteral("id1")));
        m1->mAutostart = true;
        scheduler.addMigrator(m1);
        jobTracker.mJobs.first()->suspend();
        QCOMPARE(m1->migrationState(), MigratorBase::Paused);
        jobTracker.mJobs.first()->resume();
        QCOMPARE(m1->migrationState(), MigratorBase::InProgress);
    }

    /*
     * Even if the migrator doesn't implement suspend, the executor suspends after completing the current job and waits with starting the second job.
     */
    void testJobFinishesDuringSuspend()
    {
        TestJobTracker jobTracker;
        MigrationScheduler scheduler(&jobTracker);
        QSharedPointer<Testmigrator> m1(new Testmigrator(QStringLiteral("id1")));
        m1->mAutostart = true;
        scheduler.addMigrator(m1);
        QSharedPointer<Testmigrator> m2(new Testmigrator(QStringLiteral("id2")));
        m2->mAutostart = true;
        scheduler.addMigrator(m2);
        jobTracker.mJobs.first()->suspend();
        m1->complete();
        QCOMPARE(m2->migrationState(), MigratorBase::None);
        jobTracker.mJobs.first()->resume();
        QCOMPARE(m2->migrationState(), MigratorBase::InProgress);
    }

    void testProgressReporting()
    {
        TestJobTracker jobTracker;
        MigrationScheduler scheduler(&jobTracker);
        QSharedPointer<Testmigrator> m1(new Testmigrator(QStringLiteral("id1")));
        m1->mAutostart = true;
        scheduler.addMigrator(m1);
        QCOMPARE(jobTracker.mPercent, 0);
        m1->complete();
        QCOMPARE(jobTracker.mPercent, 100);
    }
};

QTEST_MAIN(SchedulerTest)

#include "schedulertest.moc"
