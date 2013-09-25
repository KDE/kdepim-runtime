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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QTest>
#include <QSignalSpy>
#include <KDebug>

#include "../migrationscheduler.h"
#include <migration/migratorbase.h>


class Testmigrator: public MigratorBase
{
    Q_OBJECT
public:
    explicit Testmigrator(const QString &identifier, QObject *parent = 0): 
        MigratorBase(QLatin1String("testmigrator") + identifier, QString(), QString(), parent), mAutostart(false)
    {};

    virtual QString displayName() const
    {
        return QLatin1String("name");
    }

    virtual void startWork()
    {};

    virtual void abort()
    {
        setMigrationState(Aborted);
    }

    virtual void complete()
    {
        setMigrationState(Complete);
    }

    virtual bool shouldAutostart() const
    {
        return mAutostart;
    }

    bool mAutostart;
};

Q_DECLARE_METATYPE(QModelIndex)

class SchedulerTest: public QObject
{
    Q_OBJECT
private slots:

    void initTestcase()
    {
        qRegisterMetaType<QModelIndex>();
    }

    void testInsertRow()
    {
        MigrationScheduler scheduler;
        QAbstractItemModel &model(scheduler.model());

        QCOMPARE(model.rowCount(), 0);

        QSignalSpy rowsInsertedSpy(&model, SIGNAL(rowsInserted(QModelIndex,int,int)));
        QVERIFY(rowsInsertedSpy.isValid());

        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QLatin1String("id"))));
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(rowsInsertedSpy.count(), 1);

        QVERIFY(model.index(0, 0).isValid());
        QVERIFY(!model.index(1, 0).isValid());

        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QLatin1String("id2"))));
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(rowsInsertedSpy.count(), 2);
    }

    void testDisplayName()
    {
        MigrationScheduler scheduler;
        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QLatin1String("id"))));
        QAbstractItemModel &model(scheduler.model());
        QCOMPARE(model.data(model.index(0, 0)).toString(), QLatin1String("name"));
    }

    void testStartStop()
    {
        MigrationScheduler scheduler;
        QSharedPointer<Testmigrator> migrator(new Testmigrator(QLatin1String("id")));
        scheduler.addMigrator(migrator);

        scheduler.start(migrator->identifier());
        QCOMPARE(migrator->migrationState(), MigratorBase::InProgress);

        scheduler.abort(migrator->identifier());
        QCOMPARE(migrator->migrationState(), MigratorBase::Aborted);
    }

    void testNoDuplicates()
    {
        MigrationScheduler scheduler;
        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QLatin1String("id"))));
        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QLatin1String("id"))));
        QAbstractItemModel &model(scheduler.model());
        QCOMPARE(model.rowCount(), 1);
    }

    void testMigrationStateChanged()
    {
        MigrationScheduler scheduler;
        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QLatin1String("id1"))));
        QSharedPointer<Testmigrator> migrator(new Testmigrator(QLatin1String("id2")));
        scheduler.addMigrator(migrator);
        scheduler.addMigrator(QSharedPointer<Testmigrator>(new Testmigrator(QLatin1String("id3"))));
        QAbstractItemModel &model(scheduler.model());

        QSignalSpy spy(&model, SIGNAL(dataChanged(QModelIndex,QModelIndex)));
        QVERIFY(spy.isValid());
        migrator->start();

        //One for progress one for state change
        QCOMPARE(spy.count(), 2);
        const QVariantList args = spy.takeFirst();
        QCOMPARE(args.at(0).value<QModelIndex>().row(), 1);
        QCOMPARE(args.at(1).value<QModelIndex>().row(), 1);
    }

    void testRunMultiple()
    {
        MigrationScheduler scheduler;

        QSharedPointer<Testmigrator> m1(new Testmigrator(QLatin1String("id1")));
        scheduler.addMigrator(m1);

        QSharedPointer<Testmigrator> m2(new Testmigrator(QLatin1String("id2")));
        scheduler.addMigrator(m2);

        scheduler.start(m1->identifier());
        scheduler.start(m2->identifier());

        QCOMPARE(m1->migrationState(), MigratorBase::InProgress);
        QCOMPARE(m2->migrationState(), MigratorBase::InProgress);
    }

    void testRunAutostart()
    {
        MigrationScheduler scheduler;

        QSharedPointer<Testmigrator> m1(new Testmigrator(QLatin1String("id1")));
        m1->mAutostart = true;
        scheduler.addMigrator(m1);

        QSharedPointer<Testmigrator> m2(new Testmigrator(QLatin1String("id2")));
        m2->mAutostart = true;
        scheduler.addMigrator(m2);

        QCOMPARE(m1->migrationState(), MigratorBase::InProgress);
        kDebug() << m2->migrationState();
        QCOMPARE(m2->migrationState(), MigratorBase::None);
        m1->complete();
        QCOMPARE(m2->migrationState(), MigratorBase::InProgress);

    }

};

QTEST_MAIN(SchedulerTest)

#include "schedulertest.moc"
