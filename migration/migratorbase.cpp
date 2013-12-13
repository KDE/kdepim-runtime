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

#include "migratorbase.h"
#include <KStandardDirs>
#include <KDebug>
#include <KComponentData>
#include <KLocalizedString>
#include <akonadi/servermanager.h>
#include <QFile>
#include <QDateTime>

static QString messageTypeToString(MigratorBase::MessageType type)
{
    switch (type) {
        case MigratorBase::Success: return QLatin1String("Success");
        case MigratorBase::Skip:    return QLatin1String("Skipped");
        case MigratorBase::Info:    return QLatin1String("Info   ");
        case MigratorBase::Warning: return QLatin1String("WARNING");
        case MigratorBase::Error:   return QLatin1String("ERROR  ");
    }
    Q_ASSERT(false);
    return QString();
}


static QMap<QString, MigratorBase::MigrationState> fillMigrationStateMapping()
{
    QMap<QString, MigratorBase::MigrationState> map;
    map.insert(QLatin1String("Complete"), MigratorBase::Complete);
    map.insert(QLatin1String("Aborted"), MigratorBase::Aborted);
    map.insert(QLatin1String("InProgress"), MigratorBase::InProgress);
    map.insert(QLatin1String("Failed"), MigratorBase::Failed);
    return map;
}

static QMap<QString, MigratorBase::MigrationState> migrationStateMapping = fillMigrationStateMapping();

static QString stateToIdentifier(MigratorBase::MigrationState state)
{
    Q_ASSERT(migrationStateMapping.values().contains(state));
    return migrationStateMapping.key(state);
}

static MigratorBase::MigrationState identifierToState(const QString &identifier)
{
    Q_ASSERT(migrationStateMapping.contains(identifier));
    return migrationStateMapping.value(identifier);
}

MigratorBase::MigratorBase(const QString &identifier, QObject *parent)
:   QObject(parent),
    mIdentifier(identifier),
    mMigrationState(None),
    mConfig(new KConfig(Akonadi::ServerManager::addNamespace(QLatin1String("akonadi-migrationrc"))))
{
    const QString logFileName = KStandardDirs::locateLocal("data", KGlobal::mainComponent().componentName() + QLatin1String("/") + identifier + QLatin1String("migration.log"));
    setLogfile(logFileName);
    connect(this, SIGNAL(message(MigratorBase::MessageType,QString)), SLOT(logMessage(MigratorBase::MessageType,QString)));
    loadState();
}

MigratorBase::MigratorBase(const QString &identifier, const QString &configFile, const QString &logFile, QObject *parent)
:   QObject(parent),
    mIdentifier(identifier),
    mMigrationState(None)
{
    if (!configFile.isEmpty()) {
        mConfig.reset(new KConfig(configFile));
    }
    setLogfile(logFile);
    connect(this, SIGNAL(message(MigratorBase::MessageType,QString)), SLOT(logMessage(MigratorBase::MessageType,QString)));
    loadState();
}

MigratorBase::~MigratorBase()
{

}

void MigratorBase::setLogfile(const QString &logfile)
{
    if (!logfile.isEmpty()) {
        mLogFile.reset(new QFile(logfile));
        if (!mLogFile->open(QFile::Append)) {
            mLogFile.reset();
            kWarning() << "Unable to open log file: " << logfile;
        }
    } else {
        mLogFile.reset();
    }
}

QString MigratorBase::identifier() const
{
    return mIdentifier;
}

QString MigratorBase::displayName() const
{
    return QString();
}

QString MigratorBase::description() const
{
    return QString();
}

QString MigratorBase::logfile() const
{
    if (mLogFile) {
        return mLogFile->fileName();
    }
    return QString();
}

bool MigratorBase::canStart()
{
    if (mIdentifier.isEmpty()) {
        emit message(Error, i18n("Missing Identifier"));
        return false;
    }
    return true;
}

void MigratorBase::start()
{
    if (mMigrationState == InProgress) {
        kWarning() << "already running";
        return;
    }
    if (!canStart()) {
        emit message(Error, i18n("Failed to start migration because migrator is not ready"));
        emit stoppedProcessing();
        return;
    }
    //TODO acquire dbus lock
    logMessage(Info, displayName());
    emit message(Info, i18n("Starting migration..."));
    setMigrationState(InProgress);
    setProgress(0);
    startWork();
}

void MigratorBase::pause()
{
    kWarning() << "pause is not implemented";
}

void MigratorBase::resume()
{
    kWarning() << "resume is not implemented";
}

void MigratorBase::abort()
{
    kWarning() << "abort is not implemented";
}

void MigratorBase::logMessage(MigratorBase::MessageType type, const QString &msg)
{
    if (mLogFile) {
        mLogFile->write(QString(QLatin1Char('[') + QDateTime::currentDateTime().toString() + QLatin1String("] ")
        + messageTypeToString(type) + QLatin1String(": ") + msg + QLatin1Char('\n')).toUtf8());
        mLogFile->flush();
    }
}

bool MigratorBase::shouldAutostart() const
{
    return false;
}

void MigratorBase::setMigrationState(MigratorBase::MigrationState state)
{
    mMigrationState = state;
    switch (state) {
        case Complete:
            setProgress(100);
            emit message(Success, i18n("Migration complete"));
            emit stoppedProcessing();
            break;
        case Aborted:
            emit message(Skip, i18n("Migration aborted"));
            emit stoppedProcessing();
            break;
        case InProgress:
            break;
        case Failed:
            emit message(Error, i18n("Migration failed"));
            emit stoppedProcessing();
            break;
        case Paused:
            emit message(Info, i18n("Migration paused"));
            emit stateChanged(mMigrationState);
            return;
        default:
            kWarning() << "invalid state " << state;
            Q_ASSERT(false);
            return;
    }
    saveState();
    emit stateChanged(mMigrationState);
}

MigratorBase::MigrationState MigratorBase::migrationState() const
{
    return mMigrationState;
}

void MigratorBase::saveState()
{
    config().writeEntry(QLatin1String("MigrationState"), stateToIdentifier(mMigrationState));
}

void MigratorBase::loadState()
{
    const QString state = config().readEntry(QLatin1String("MigrationState"), QString());
    if (!state.isEmpty()) {
        mMigrationState = identifierToState(state);
    }

    if (mMigrationState == InProgress) {
        emit message(Warning, i18n("This migration has already been started once but was aborted"));
        mMigrationState = NeedsUpdate;
    }
    switch (mMigrationState) {
        case Complete:
            mProgress = 100;
            break;
        default:
            mProgress = 0;
    }
}

NullableConfigGroup MigratorBase::config()
{
    if (mConfig) {
        return NullableConfigGroup(mConfig->group(mIdentifier));
    }
    return NullableConfigGroup();
}

int MigratorBase::progress() const
{
    return mProgress;
}

void MigratorBase::setProgress(int prog)
{
    if (mProgress != prog) {
        mProgress = prog;
        emit progress(prog);
    }
}

QString MigratorBase::status() const
{
    switch (mMigrationState) {
        case None: return i18nc("@info:status", "Not started");
        case InProgress: return i18nc("@info:status", "Running...");
        case Complete: return i18nc("@info:status", "Complete");
        case Aborted: return i18nc("@info:status", "Aborted");
        case Paused: return i18nc("@info:status", "Paused");
        case NeedsUpdate: return i18nc("@info:status", "Needs Update");
        case Failed: return i18nc("@info:status", "Failed");
    }
    return QString();
}

