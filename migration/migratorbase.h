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

#ifndef MIGRATORBASE_H
#define MIGRATORBASE_H

#include <QtCore/QObject>
#include <QFile>
#include <kconfig.h>
#include <KConfigGroup>

class QFile;

class NullableConfigGroup
{
public:
    NullableConfigGroup()
    {};

    NullableConfigGroup(KConfigGroup grp): mConfigGroup(grp)
    {};

    KConfigGroup &configGroup()
    {
        return mConfigGroup; 
    };

    template <typename T>
    inline T readEntry(const QString &key, const T &aDefault) const
    {
        if (mConfigGroup.isValid()) {
            return mConfigGroup.readEntry<T>(key, aDefault);
        }
        return aDefault;
    }

    template <typename T>
    inline void writeEntry(const QString &key, const T &value)
    {
        if (mConfigGroup.isValid()) {
            mConfigGroup.writeEntry<T>(key, value);
        }
    }
private:
    KConfigGroup mConfigGroup;
};

/**
 * Base class for generic migration jobs in akonadi.
 *
 * MigrationJobs can be run standalone from commandline using a small wrapper application or using the
 * Akonadi Migration Agent.
 *
 * Each migrator should assign a unique identifier for it's state (this identifier must never change).
 *
 * The work done by the migrator may be paused, and the migrator may persist it's state to resume migrations after a reboot.
 *
 * TODO: The migrator base ensures that no migrator can be run multiple times by locking it over dbus.
 *
 * The status is stored in the akonadi instance config directory, meaning the status is stored per akonadi instance.
 * This is the only reason why this MigratorBase is currently specific to akonadi migration jobs.
 */
class MigratorBase : public QObject
{
    Q_OBJECT
public:
    /**
     * Default constructor with default config and logfile
     */
    explicit MigratorBase(const QString &identifier, QObject *parent = 0);

    /**
     * Constructor that allows to inject a configfile and logfile.
     *
     * Pass and empty string to disable config and log.
     */
    explicit MigratorBase(const QString &identifier, const QString &configFile, const QString &logFile, QObject *parent = 0);

    virtual ~MigratorBase();

    QString identifier() const;

    /**
     * Translated, human readable display name of migrator.
     */
    virtual QString displayName() const;

    /**
     * Translated, human readable description of migrator.
     */
    virtual QString description() const;

    /**
     * Returns the filename of the logfile used by this migrator.
     *
     * Returns QString() if there is no logfile set.
     */
    QString logfile() const;

    enum MigrationState {
        None,
        InProgress,
        Paused,
        Complete,
        NeedsUpdate,
        Aborted,
        Failed
    };

    enum MessageType {
        Success,
        Skip,
        Info,
        Warning,
        Error
    };

    /**
     * Read migration state.
     *
     * @return MigrationState.
     */
    MigrationState migrationState() const;

    /**
     * Return false if this job cannot start (i.e. due to missing dependencies).
     */
    virtual bool canStart();

    /**
     * Mandatory updates that the Migration Agent should autostart should return true
     */
    virtual bool shouldAutostart() const;

    /**
     * Start/Continue migration.
     *
     * Implement startWork instead.
     *
     * Note that this will directly (blocking) call startWork().
     */
    void start();

    /**
     * Pause migration.
     */
    virtual void pause();

    /**
     * Abort migration.
     */
    virtual void abort();

    /**
     * progress in percent
     */
    int progress() const;

    /**
     * Status
     */
    QString status() const;

signals:
    //Signal for state changes
    void stateChanged(MigratorBase::MigrationState);

    //Signal for log window
    void message(MigratorBase::MessageType type, const QString &msg);

    //Signal for progress bar
    void progress(int progress);

    //Signal for scheduling
    void stoppedProcessing();

protected:
    /**
     * Reimplement to start work.
     */
    virtual void startWork() = 0;

    void setMigrationState(MigratorBase::MigrationState state);

    void setProgress(int);

private slots:
    /**
     * Logs a message, that appears in the logfile and potentially in a log window.
     * Do not call this directly. Emit the message signal instead, which is connected to this slot.
     */
    void logMessage(MigratorBase::MessageType type, const QString &msg);

private:
    NullableConfigGroup config();
    void saveState();
    void loadState();

    void setLogfile(const QString &);

    const QString mIdentifier;
    MigrationState mMigrationState;
    QScopedPointer<QFile> mLogFile;
    QScopedPointer<KConfig> mConfig;
    int mProgress;
};

Q_DECLARE_METATYPE(MigratorBase::MigrationState)

#endif // MIGRATORBASE_H
