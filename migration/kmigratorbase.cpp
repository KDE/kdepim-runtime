/*
    Copyright (c) 2009 Jonathan Armond <jon.armond@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "kmigratorbase.h"

#include <AkonadiCore/agentinstancecreatejob.h>
#include <AkonadiCore/agentmanager.h>
#include <AkonadiCore/agenttype.h>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QMetaEnum>
#include <QTimer>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

using namespace Akonadi;

namespace {
QString messageTypeToString(KMigratorBase::MessageType type)
{
    switch (type) {
    case KMigratorBase::Success:
        return QStringLiteral("Success");
    case KMigratorBase::Skip:
        return QStringLiteral("Skipped");
    case KMigratorBase::Info:
        return QStringLiteral("Info   ");
    case KMigratorBase::Warning:
        return QStringLiteral("WARNING");
    case KMigratorBase::Error:
        return QStringLiteral("ERROR  ");
    }
    Q_ASSERT(false);
    return QString();
}
}

KMigratorBase::KMigratorBase() : m_logFile(nullptr)
{
    const QString logFileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + QCoreApplication::applicationName() + QLatin1String("/migration.log");
    QFileInfo fileInfo(logFileName);
    QDir().mkpath(fileInfo.absolutePath());

    m_logFile = new QFile(logFileName);
    if (!m_logFile->open(QFile::Append)) {
        delete m_logFile;
        m_logFile = nullptr;
        qWarning() << "Unable to open log file: " << logFileName;
    }
    logMessage(Info, QStringLiteral("Starting migration..."));
    connect(this, &KMigratorBase::message, this, &KMigratorBase::logMessage);

    // load the vtable before we continue
    QTimer::singleShot(0, this, &KMigratorBase::migrate);
}

KMigratorBase::~KMigratorBase()
{
    logMessage(Info, QStringLiteral("Migration finished."));
    delete m_logFile;
}

KMigratorBase::MigrationState KMigratorBase::migrationState(const QString &identifier) const
{
    KConfigGroup cfg(KSharedConfig::openConfig(), QStringLiteral("Resource ") + identifier);
    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("MigrationState"));
    const QString s = cfg.readEntry("MigrationState", e.valueToKey(None));
    MigrationState state = (MigrationState)e.keyToValue(s.toLatin1().constData());

    if (state != None) {
        const QString resId = cfg.readEntry("ResourceIdentifier", "");
        // previously migrated but removed again
        if (!AgentManager::self()->instance(resId).isValid()) {
            state = None;
        }
    }

    return state;
}

void KMigratorBase::setMigrationState(const QString &identifier, MigrationState state, const QString &resId, const QString &type)
{
    KConfigGroup cfg(KSharedConfig::openConfig(), QStringLiteral("Resource ") + identifier);
    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("MigrationState"));
    const QString stateStr = QLatin1String(e.valueToKey(state));
    cfg.writeEntry("MigrationState", stateStr);
    cfg.writeEntry("ResourceIdentifier", resId);
    cfg.sync();

    cfg = KConfigGroup(KSharedConfig::openConfig(), "Bridged");
    QStringList bridgedResources = cfg.readEntry(type + QLatin1String("Resources"), QStringList());
    if (state == Bridged) {
        if (!bridgedResources.contains(identifier)) {
            bridgedResources << identifier;
        }
    } else {
        bridgedResources.removeAll(identifier);
    }
    cfg.writeEntry(type + QLatin1String("Resources"), bridgedResources);
    cfg.sync();
}

KJob *KMigratorBase::createAgentInstance(const QString &typeId, QObject *receiver, const char *slot)
{
    Q_EMIT message(Info, i18n("Creating instance of type %1", typeId));
    AgentInstanceCreateJob *job = new AgentInstanceCreateJob(typeId, this);
    connect(job, SIGNAL(result(KJob*)), receiver, slot);
    job->start();
    return job;
}

void KMigratorBase::logMessage(KMigratorBase::MessageType type, const QString &msg)
{
    if (m_logFile) {
        m_logFile->write(QString(QLatin1Char('[') + QDateTime::currentDateTime().toString() + QLatin1String("] ")
                                 + messageTypeToString(type) + QLatin1String(": ") + msg + QLatin1Char('\n')).toUtf8());
        m_logFile->flush();
    }
}
