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
#include <QDir>

#include "migratorbase.h"

class Testmigrator: public MigratorBase
{
    Q_OBJECT
public:
    bool mCanStart;

    explicit Testmigrator(const QString &identifier, QObject *parent = 0)
        : MigratorBase(QLatin1String("testmigrator") + identifier, parent),
        mCanStart(true)
    {};

    explicit Testmigrator(const QString &identifier, const QString &logfile, const QString &configFile, QObject *parent = 0)
        : MigratorBase(QLatin1String("testmigrator") + identifier, configFile, logfile, parent),
        mCanStart(true)
    {};

    virtual bool canStart()
    {
        return mCanStart;
    }

    virtual void startWork()
    {
        emit started();
    }

    virtual void pause()
    {
        setMigrationState(Paused);
    }

    virtual void abort()
    {
        setMigrationState(Aborted);
    }

    void setComplete()
    {
        setMigrationState(Complete);
    }

signals:
    void started();
};

class MigratorBaseTest: public QObject
{
    Q_OBJECT
private slots:

    void initTest()
    {
        qRegisterMetaType<MigratorBase::MigrationState>();
    }

    void testStart()
    {
        Testmigrator migrator(QLatin1String("id"));
        {
            QSignalSpy spy(&migrator, SIGNAL(started()));
            QSignalSpy stateSpy(&migrator, SIGNAL(stateChanged(MigratorBase::MigrationState)));
            QVERIFY(spy.isValid());
            migrator.mCanStart = false;
            migrator.start();
            QCOMPARE(spy.count(), 0);
            QCOMPARE(stateSpy.count(), 0);
            migrator.mCanStart = true;
            migrator.start();
            QCOMPARE(spy.count(), 1);
            QCOMPARE(stateSpy.count(), 1);
            QCOMPARE(stateSpy.takeFirst().at(0).value<MigratorBase::MigrationState>(), MigratorBase::InProgress);
        }
        {
            QSignalSpy spy(&migrator, SIGNAL(stateChanged(MigratorBase::MigrationState)));
            QVERIFY(spy.isValid());
            migrator.abort();
            QCOMPARE(spy.count(), 1);
            QCOMPARE(spy.takeFirst().at(0).value<MigratorBase::MigrationState>(), MigratorBase::Aborted);
        }
        {
            QSignalSpy spy(&migrator, SIGNAL(stateChanged(MigratorBase::MigrationState)));
            QVERIFY(spy.isValid());
            migrator.setComplete();
            QCOMPARE(spy.count(), 1);
            QCOMPARE(spy.takeFirst().at(0).value<MigratorBase::MigrationState>(), MigratorBase::Complete);
        }
    }

    void testDisableLogAndConfig()
    {
        Testmigrator migrator(QLatin1String("id"), QString(), QString());
        migrator.start();
        migrator.setComplete();
    }

    void testPersistState()
    {
        const QString configFile(QDir::tempPath() + QLatin1String("/testconfig.cfg"));
        QFile file(configFile);
        file.remove();
        {
            Testmigrator migrator(QLatin1String("id"), QString(), configFile);
            QCOMPARE(migrator.migrationState(), MigratorBase::None);
            migrator.setComplete();
        }
        {
            Testmigrator migrator(QLatin1String("id"), QString(), configFile);
            QCOMPARE(migrator.migrationState(), MigratorBase::Complete);
        }
    }

};

QTEST_MAIN(MigratorBaseTest)

#include "testmigratorbase.moc"
