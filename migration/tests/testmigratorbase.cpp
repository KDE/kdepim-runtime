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
        : MigratorBase("testmigrator" + identifier, parent),
        mCanStart(true)
    {};

    explicit Testmigrator(const QString &identifier, const QString &logfile, const QString &configFile, QObject *parent = 0)
        : MigratorBase("testmigrator" + identifier, configFile, logfile, parent),
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
