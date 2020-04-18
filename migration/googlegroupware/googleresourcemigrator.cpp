/*
    Copyright (c) 2020 Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "googleresourcemigrator.h"
#include "googlesettingsinterface.h"
#include "migration_debug.h"

#include <AkonadiCore/AgentManager>
#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/ServerManager>
#include <AkonadiCore/AgentInstanceCreateJob>

#include <KLocalizedString>
#include <KWallet/KWallet>

#include <KWallet/kwallet.h>
#include <QSettings>

#include <memory>

GoogleResourceMigrator::GoogleResourceMigrator()
    : MigratorBase(QLatin1String("googleresourcemigrator"))
{}

QString GoogleResourceMigrator::displayName() const
{
    return i18nc("Name of the Migrator (intended for advanced users).", "Google Resource Migrator");
}

QString GoogleResourceMigrator::description() const
{
    return i18nc("Description of the migrator",
                 "Migrates the old Google Calendar and Google Contacts resources to the new unified Google Groupware Resource");
}

bool GoogleResourceMigrator::shouldAutostart() const
{
    return true;
}

namespace {

static const QStringView akonadiGoogleCalendarResource = {u"akonadi_googlecalendar_resource"};
static const QStringView akonadiGoogleContactsResource = {u"akonadi_googlecontacts_resource"};
static const QStringView akonadiGoogleGroupwareResource = {u"akonadi_google_resource"};

bool isLegacyGoogleResource(const Akonadi::AgentInstance &instance)
{
    return instance.type().identifier() == akonadiGoogleCalendarResource
            || instance.type().identifier() == akonadiGoogleContactsResource;
}

bool isGoogleGroupwareResource(const Akonadi::AgentInstance &instance)
{
    return instance.type().identifier() == akonadiGoogleGroupwareResource;
}

std::unique_ptr<QSettings> settingsForResource(const Akonadi::AgentInstance &instance)
{
    const auto configFile = Akonadi::ServerManager::self()->addNamespace(instance.identifier()) + QStringLiteral("rc");
    const auto configPath = QStandardPaths::locate(QStandardPaths::ConfigLocation, configFile);
    return std::unique_ptr<QSettings>{new QSettings{configPath, QSettings::IniFormat}};
}

QString getAccountNameFromResourceSettings(const Akonadi::AgentInstance &instance)
{
    const auto config = settingsForResource(instance);
    QString account = config->value(QStringLiteral("Account")).toString();
    if (account.isEmpty()) {
        account = config->value(QStringLiteral("AccountName")).toString();
    }

    return account;
}

static const auto WalletFolder = QStringLiteral("Akonadi Google");

std::unique_ptr<KWallet::Wallet> getWallet()
{
    std::unique_ptr<KWallet::Wallet> wallet{KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous)};
    if (!wallet) {
        qCWarning(MIGRATION_LOG) << "GoogleResourceMigrator: failed to open KWallet.";
        return {};
    }

    if (!wallet->hasFolder(WalletFolder)) {
        qCWarning(MIGRATION_LOG) << "GoogleResourceMigrator: couldn't find wallet folder for Google resources.";
        return {};
    }
    wallet->setFolder(WalletFolder);

    return wallet;
}

QMap<QString, QString> backupKWalletData(const QString &account)
{
    qCInfo(MIGRATION_LOG) << "GoogleResourceMigrator: backing up KWallet data for" << account;

    const auto wallet = getWallet();
    if (!wallet) {
        return {};
    }

    if (!wallet->entryList().contains(account)) {
        qCWarning(MIGRATION_LOG) << "GoogleResourceMigrator: couldn't find KWallet data for account" << account;
        return {};
    }

    QMap<QString, QString> map;
    wallet->readMap(account, map);
    return map;
}

void restoreKWalletData(const QString &account, const QMap<QString, QString> &data)
{
    qCInfo(MIGRATION_LOG) << "GoogleResourceMigrator: restoring KWallet data for" << account;

    auto wallet = getWallet();
    if (!wallet) {
        return;
    }

    wallet->writeMap(account, data);
}

} // namespace

void GoogleResourceMigrator::startWork()
{
    // Discover all existing Google Contacts and Google Calendar resources
    const auto allInstances = Akonadi::AgentManager::self()->instances();
    for (const auto &instance : allInstances) {
        if (isLegacyGoogleResource(instance)) {
            const auto account = getAccountNameFromResourceSettings(instance);
            if (account.isEmpty()) {
                qCInfo(MIGRATION_LOG) << "GoogleResourceMigrator: resource" << instance.identifier() << "is not configued, removing";
                Akonadi::AgentManager::self()->removeInstance(instance);
                continue;
            }
            qCInfo(MIGRATION_LOG) << "GoogleResourceMigrator: discovered resource" << instance.identifier()
                                  << "for account" << account;
            if (instance.type().identifier() == akonadiGoogleCalendarResource) {
                mMigrations[account].calendarResource = instance;
            } else if (instance.type().identifier() == akonadiGoogleContactsResource) {
                mMigrations[account].contactResource = instance;
            }
        } else if (isGoogleGroupwareResource(instance)) {
            const auto account = getAccountNameFromResourceSettings(instance);
            mMigrations[account].alreadyExists = true;
        }
    }

    mMigrationCount = mMigrations.size();
    migrateNextAccount();
}

void GoogleResourceMigrator::removeLegacyInstances(const Instances &instances)
{

    if (instances.calendarResource.isValid()) {
        Akonadi::AgentManager::self()->removeInstance(instances.calendarResource);
        qCInfo(MIGRATION_LOG) << "GoogleResourceMigrator: removed the legacy calendar resource" << instances.calendarResource.identifier();
    }
    if (instances.contactResource.isValid()) {
        Akonadi::AgentManager::self()->removeInstance(instances.contactResource);
        qCInfo(MIGRATION_LOG) << "GoogleResourceMigrator: removed the legacy contacts resource" << instances.contactResource.identifier();
    }
}

void GoogleResourceMigrator::migrateNextAccount()
{
    setProgress((static_cast<float>(mMigrationsDone) / mMigrationCount) * 100);
    if (mMigrations.empty()) {
        setMigrationState(MigratorBase::Complete);
        return;
    }

    QString account;
    Instances instances;
    std::tie(account, instances) = *mMigrations.constKeyValueBegin();
    mMigrations.erase(mMigrations.begin());

    if (instances.alreadyExists) {
        message(Info, i18n("Google Groupware Resource for account %1 already exists, skipping.", account));
        // Just to be sure, check that there are no left-over legacy instances
        removeLegacyInstances(instances);

        ++mMigrationsDone;
        QMetaObject::invokeMethod(this, &GoogleResourceMigrator::migrateNextAccount, Qt::QueuedConnection);
        return;
    }

    qCInfo(MIGRATION_LOG) << "GoogleResourceMigrator: starting migration of account" << account;
    message(Info, i18n("Starting migration of account %1", account));
    qCInfo(MIGRATION_LOG) << "GoogleResourceMigrator: creating new" << akonadiGoogleGroupwareResource;
    message(Info, i18n("Creating new instance of Google Gropware Resource"));
    auto job = new Akonadi::AgentInstanceCreateJob(akonadiGoogleGroupwareResource.toString(), this);
    connect(job, &Akonadi::AgentInstanceCreateJob::finished,
            this, [this, job, account, instances](KJob *) {
                if (job->error()) {
                    qCWarning(MIGRATION_LOG) << "GoogleResourceMigrator: Failed to create new Google Groupware Resource:" << job->errorString();
                    message(Error, i18n("Failed to create a new Google Groupware Resource: %1", job->errorString()));
                    setMigrationState(MigratorBase::Failed);
                    return;
                }

                const auto newInstance = job->instance();
                if (!migrateAccount(account, instances, newInstance)) {
                    qCWarning(MIGRATION_LOG) << "GoogleResourceMigrator: failed to migrate account" << account;
                    message(Error, i18n("Failed to migrate account %1", account));
                    setMigrationState(MigratorBase::Failed);
                    return;
                }

                // Legacy resources wipe KWallet data when removed, so we need to back the data up
                // before removing them and restore it afterwards
                const auto kwalletData = backupKWalletData(account);

                removeLegacyInstances(instances);

                restoreKWalletData(account, kwalletData);

                // Reconfigure and restart the new instance
                newInstance.reconfigure();
                newInstance.restart();

                if (instances.calendarResource.isValid() ^ instances.contactResource.isValid()) {
                    const auto res = instances.calendarResource.isValid()
                                        ? instances.calendarResource.identifier()
                                        : instances.contactResource.identifier();
                    qCInfo(MIGRATION_LOG) << "GoogleResourceMigrator: migrated configuration from" << res
                                           << "to" << newInstance.identifier();
                } else {
                    qCInfo(MIGRATION_LOG) << "GoogleResourceMigrator: migrated configuration from"
                                           << instances.calendarResource.identifier() << "and"
                                           << instances.contactResource.identifier() << "to"
                                           << newInstance.identifier();
                }
                message(Success, i18n("Migrated account %1 to new Google Groupware Resource", account));

                ++mMigrationsDone;
                migrateNextAccount();
            });
    job->start();
}

bool GoogleResourceMigrator::migrateAccount(const QString &account, const Instances &oldInstances,
                                            const Akonadi::AgentInstance &newInstance)
{
    org::kde::Akonadi::Google::Settings resourceSettings{
        Akonadi::ServerManager::self()->agentServiceName(Akonadi::ServerManager::Resource, newInstance.identifier()),
        QStringLiteral("/Settings"), QDBusConnection::sessionBus()};
    if (!resourceSettings.isValid()) {
        qCWarning(MIGRATION_LOG) << "GoogleResourceMigrator: failed to obtain settings DBus interface of " << newInstance.identifier();
        return false;
    }

    resourceSettings.setAccount(account);

    // Calendar-specific
    const auto calendarSettings = settingsForResource(oldInstances.calendarResource);
    resourceSettings.setCalendars(calendarSettings->value(QStringLiteral("Calendars")).toStringList());
    resourceSettings.setTaskLists(calendarSettings->value(QStringLiteral("TaskLists")).toStringList());
    resourceSettings.setEventsSince(calendarSettings->value(QStringLiteral("EventsSince")).toString());
    resourceSettings.setEnableIntervalCheck(calendarSettings->value(QStringLiteral("EnableIntervalCheck"), false).toBool());
    resourceSettings.setIntervalCheckTime(calendarSettings->value(QStringLiteral("IntervalCheckTime"), 60).toInt());

    // KAccounts support, should be common in both legacy resources when set
    resourceSettings.setAccountName(calendarSettings->value(QStringLiteral("AccountName")).toString());
    resourceSettings.setAccountId(calendarSettings->value(QStringLiteral("AccountId"), 0).toInt());

    resourceSettings.save();

    return true;
};
