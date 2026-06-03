/*
    SPDX-FileCopyrightText: 2011-2013 Dan Vratil <dan@progdan.cz>
    SPDX-FileCopyrightText: 2020 Igor Poboiko <igor.poboiko@gmail.com>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "googlesettings.h"
#include "googleresource_debug.h"
#include "googlescopes.h"
#include "settingsadaptor.h"

#include <KGAPI/Account>
#include <KLocalizedString>

#include <QDataStream>
#include <QIODevice>

#include <qt6keychain/keychain.h>
using namespace QKeychain;

using namespace KGAPI2;

using namespace Qt::Literals;

static const QString googleWalletFolder = QStringLiteral("Akonadi Google");

GoogleSettings::GoogleSettings(const KSharedConfigPtr &config, Options options)
    : SettingsBase(config)
{
    if (options & Option::ExportToDBus) {
        new SettingsAdaptor(this);
        QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                     this,
                                                     QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents);
    }
}

void GoogleSettings::init()
{
    if (account().isEmpty()) {
        qCWarning(GOOGLE_LOG) << Q_FUNC_INFO << "No username set";
        Q_EMIT accountReady(false, i18nc("@info:status", "No username set"));
        return;
    }

    if (!m_accountId.isEmpty()) {
        qCDebug(GOOGLE_LOG) << "Online account detected, reading credentials from DBus";

        initOnlineAccount();

        Q_EMIT accountReady(true);
    } else {
        qCWarning(GOOGLE_LOG) << "Trying to read password for" << account();

        // First read from QtKeyChain
        auto job = new QKeychain::ReadPasswordJob(googleWalletFolder);
        job->setKey(account());
        connect(job, &QKeychain::Job::finished, this, [this, job]() {
            if (job->error() != QKeychain::Error::NoError) {
                qCWarning(GOOGLE_LOG) << "Unable to read password:" << job->errorString();
                Q_EMIT accountReady(false, i18nc("@info:status", "Unable to read password: %1", job->errorString()));
                return;
            }

            // Found something with QtKeyChain
            m_account = fetchAccountFromKeychain(account(), job);
            m_isReady = true;
            Q_EMIT accountReady(true);
        });
        job->start();
    }
}

void GoogleSettings::setAccountId(const QString &accountId)
{
    m_accountId = accountId;
}

KGAPI2::AccountPtr GoogleSettings::fetchAccountFromKeychain(const QString &accountName, QKeychain::ReadPasswordJob *job)
{
    QMap<QString, QString> map;
    auto value = job->binaryData();
    if (value.isEmpty()) {
        qCWarning(GOOGLE_LOG) << "Account" << accountName << "not found in password store";
        return {};
    }

    QDataStream ds(value);
    ds >> map;

    const QStringList scopes = map[QStringLiteral("scopes")].split(u',', Qt::SkipEmptyParts);
    QList<QUrl> scopeUrls;
    scopeUrls.reserve(scopes.count());
    for (const QString &scope : scopes) {
        scopeUrls << QUrl(scope);
    }
    AccountPtr account(new Account(accountName, map[QStringLiteral("accessToken")], map[QStringLiteral("refreshToken")], scopeUrls));
    return account;
}

WritePasswordJob *GoogleSettings::storeAccount(AccountPtr account)
{
    // Removing the old one (if present)
    if (m_account && (account->accountName() != m_account->accountName())) {
        cleanup();
    }
    // Populating the new one
    m_account = account;

    QStringList scopes;
    const QList<QUrl> urlScopes = googleScopes();
    scopes.reserve(urlScopes.count());
    for (const QUrl &url : urlScopes) {
        scopes << url.toString();
    }

    const QMap<QString, QString> map = {
        {QStringLiteral("accessToken"), m_account->accessToken()},
        {QStringLiteral("refreshToken"), m_account->refreshToken()},
        {QStringLiteral("scopes"), scopes.join(u',')},
    };

    // Legacy: keep the serialized map format unchanged for compatibility
    QByteArray mapData;
    QDataStream ds(&mapData, QIODevice::WriteOnly);
    ds << map;

    auto writeJob = new WritePasswordJob(googleWalletFolder);
    writeJob->setKey(m_account->accountName());
    writeJob->setBinaryData(mapData);

    connect(writeJob, &WritePasswordJob::finished, this, [this, writeJob]() {
        if (writeJob->error() != QKeychain::Error::NoError) {
            qCWarning(GOOGLE_LOG) << "Unable to write password:" << writeJob->errorString();
            return;
        }
        SettingsBase::setAccount(m_account->accountName());
        m_isReady = true;
    });

    return writeJob;
}

void GoogleSettings::initOnlineAccount()
{
    Q_ASSERT(!m_accountId.isEmpty());

    QDBusMessage propertiesRequest =
        QDBusMessage::createMethodCall(u"org.kde.KOnlineAccounts"_s, m_accountId, u"org.freedesktop.DBus.Properties"_s, u"GetAll"_s);
    propertiesRequest.setArguments({u"org.kde.KOnlineAccounts.Google"_s});

    QDBusReply<QVariantMap> propertiesReply = QDBusConnection::sessionBus().call(propertiesRequest);

    if (!propertiesReply.isValid()) {
        qCWarning(GOOGLE_LOG) << "Failed to read Google account properties" << propertiesReply.error().message();
    }

    const QVariantMap result = propertiesReply.value();

    const QString accountName = result[u"accountName"_s].toString();
    const QList<QUrl> scopes = QUrl::fromStringList(result[u"scopes"_s].toStringList());

    QDBusMessage accessTokenRequest =
        QDBusMessage::createMethodCall(u"org.kde.KOnlineAccounts"_s, m_accountId, u"org.kde.KOnlineAccounts.Google"_s, u"accessToken"_s);

    QDBusReply<QDBusUnixFileDescriptor> accessTokenReply = QDBusConnection::sessionBus().call(accessTokenRequest);

    if (!accessTokenReply.isValid()) {
        qCWarning(GOOGLE_LOG) << "Failed to read access token for Google account" << accessTokenReply.error().message();
    }

    QFile accessTokenFile;
    const bool accessTokenOpenResult = accessTokenFile.open(accessTokenReply.value().fileDescriptor(), QFile::ReadOnly, QFile::AutoCloseHandle);

    if (!accessTokenOpenResult) {
        qCWarning(GOOGLE_LOG) << "Could not open access token fd" << accessTokenFile.errorString();
        return;
    }

    const QString accessToken = QString::fromUtf8(accessTokenFile.readAll());

    Q_ASSERT(!accessToken.isEmpty());

    QDBusMessage refreshTokenRequest =
        QDBusMessage::createMethodCall(u"org.kde.KOnlineAccounts"_s, m_accountId, u"org.kde.KOnlineAccounts.Google"_s, u"refreshToken"_s);

    QDBusReply<QDBusUnixFileDescriptor> refreshTokenReply = QDBusConnection::sessionBus().call(refreshTokenRequest);

    if (!refreshTokenReply.isValid()) {
        qCWarning(GOOGLE_LOG) << "Failed to read refresh token for Google account" << refreshTokenReply.error().message();
    }

    QFile file;
    const bool refreshTokenOpenResult = file.open(refreshTokenReply.value().fileDescriptor(), QFile::ReadOnly, QFile::AutoCloseHandle);

    if (!refreshTokenOpenResult) {
        qCWarning(GOOGLE_LOG) << "Could not open refresh token fd" << file.errorString();
        return;
    }

    const QString refreshToken = QString::fromUtf8(file.readAll());

    Q_ASSERT(!refreshToken.isEmpty());

    KGAPI2::AccountPtr account(new KGAPI2::Account(accountName, accessToken, refreshToken, scopes));
    m_account = account;

    SettingsBase::setAccount(m_account->accountName());
    m_isReady = true;
}

void GoogleSettings::cleanup()
{
    if (m_account) {
        auto deleteJob = new DeletePasswordJob(googleWalletFolder);
        deleteJob->setKey(m_account->accountName());
        deleteJob->start();
    }
}

void GoogleSettings::addCalendar(const QString &calendar)
{
    if (calendars().isEmpty() || calendars().contains(calendar)) {
        return;
    }
    setCalendars(calendars() << calendar);
    save();
}

void GoogleSettings::addTaskList(const QString &taskList)
{
    if (calendars().isEmpty() || taskLists().contains(taskList)) {
        return;
    }
    setTaskLists(taskLists() << taskList);
    save();
}

QString GoogleSettings::clientId() const
{
    return QStringLiteral("554041944266.apps.googleusercontent.com");
}

QString GoogleSettings::clientSecret() const
{
    return QStringLiteral("mdT1DjzohxN3npUUzkENT0gO");
}

bool GoogleSettings::isReady() const
{
    return m_isReady;
}

AccountPtr GoogleSettings::accountPtr()
{
    return m_account;
}

#include "moc_googlesettings.cpp"
