/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "settings.h"
#include "imapaccount.h"
#include "settingsadaptor.h"
#include <config-imap.h>
#include <qt6keychain/keychain.h>

#include "imapresource_debug.h"

#include <QDBusConnection>

#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionModifyJob>

#include <KLocalizedString>
#include <KNotification>

#include <qt6keychain/keychain.h>

using namespace QKeychain;

/**
 * Maps the enum used to represent authentication in MailTransport (kdepimlibs)
 * to the one used by the imap resource.
 * @param authType the MailTransport auth enum value
 * @return the corresponding KIMAP auth value.
 * @note will cause fatal error if there is no mapping, so be careful not to pass invalid auth options (e.g., APOP) to this function.
 */
KIMAP::LoginJob::AuthenticationMode Settings::mapTransportAuthToKimap(MailTransport::Transport::EnumAuthenticationType authType)
{
    // typedef these for readability
    using MTAuth = MailTransport::Transport::EnumAuthenticationType;
    using KIAuth = KIMAP::LoginJob;
    switch (authType) {
    case MTAuth::ANONYMOUS:
        return KIAuth::Anonymous;
    case MTAuth::PLAIN:
        return KIAuth::Plain;
    case MTAuth::NTLM:
        return KIAuth::NTLM;
    case MTAuth::LOGIN:
        return KIAuth::Login;
    case MTAuth::GSSAPI:
        return KIAuth::GSSAPI;
    case MTAuth::DIGEST_MD5:
        return KIAuth::DigestMD5;
    case MTAuth::CRAM_MD5:
        return KIAuth::CramMD5;
    case MTAuth::CLEAR:
        return KIAuth::ClearText;
    case MTAuth::XOAUTH2:
        return KIAuth::XOAuth2;
    default:
        qFatal("mapping from Transport::EnumAuthenticationType ->  KIMAP::LoginJob::AuthenticationMode not possible");
    }
    return KIAuth::ClearText; // dummy value, shouldn't get here.
}

Settings::Settings(WId winId)
    : SettingsBase()
    , m_winId(winId)
{
    load();

    new SettingsAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                 this,
                                                 QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents);
}

void Settings::setWinId(WId winId)
{
    m_winId = winId;
}

void Settings::clearCachedPassword()
{
    m_password.clear();
}

void Settings::cleanup()
{
    auto deleteJob = new DeletePasswordJob(QStringLiteral("imap"));
    deleteJob->setKey(config()->name());
    deleteJob->start();

    auto deleteSieveJob = new DeletePasswordJob(QStringLiteral("imap"));
    deleteSieveJob->setKey(QLatin1StringView("custom_sieve_") + config()->name());
    deleteSieveJob->start();
}

bool Settings::mustFetchPassword() const
{
    return m_password.isEmpty() && (mapTransportAuthToKimap((MailTransport::TransportBase::EnumAuthenticationType)authentication()) != KIMAP::LoginJob::GSSAPI);
}

ReadPasswordJob *Settings::requestPassword()
{
    Q_ASSERT(mustFetchPassword());

    auto readPasswordJob = new ReadPasswordJob{QStringLiteral("imap"), this};
    readPasswordJob->setKey(config()->name());

    connect(readPasswordJob, &ReadPasswordJob::finished, this, [this, readPasswordJob](auto) {
        if (readPasswordJob->error() != NoError && readPasswordJob->error() != EntryNotFound) {
            handleError(
                i18nc("@info:status", "An error occured when retriving the IMAP password from the system keychain: \"%1\"", readPasswordJob->errorString()));
            return;
        }
        m_password = readPasswordJob->textData();
    });

    readPasswordJob->start();
    return readPasswordJob;
}

QString Settings::password() const
{
    Q_ASSERT(!mustFetchPassword());
    return m_password;
}

bool Settings::mustFetchSievePassword() const
{
    return m_customSievePassword.isEmpty();
}

ReadPasswordJob *Settings::requestSieveCustomPassword()
{
    auto readPasswordJob = new ReadPasswordJob{QStringLiteral("imap"), this};
    readPasswordJob->setKey(QLatin1StringView("custom_sieve_") + config()->name());

    connect(readPasswordJob, &ReadPasswordJob::finished, this, [this, readPasswordJob](auto) {
        if (readPasswordJob->error() != NoError && readPasswordJob->error() != EntryNotFound) {
            handleError(
                i18nc("@info:status", "An error occured when retriving the sieve password from the system keychain: \"%1\"", readPasswordJob->errorString()));
            return;
        }
        m_customSievePassword = readPasswordJob->textData();
    });

    return readPasswordJob;
}

QString Settings::sievePassword() const
{
    Q_ASSERT(!mustFetchSievePassword());
    return m_customSievePassword;
}

void Settings::setSieveCustomPassword(const QString &password)
{
    if (m_customSievePassword == password) {
        return;
    }

    m_customSievePassword = password;

    auto writePasswordJob = new WritePasswordJob{QStringLiteral("imap"), this};
    writePasswordJob->setKey(QLatin1StringView("custom_sieve_") + config()->name());
    writePasswordJob->setTextData(password);

    connect(writePasswordJob, &WritePasswordJob::finished, this, [this, writePasswordJob](auto) {
        if (writePasswordJob->error() != Error::NoError) {
            handleError(
                i18nc("@info:status", "An error occured when saving the sieve password in the system keychain: \"%1\"", writePasswordJob->errorString()));
            return;
        }
    });
    writePasswordJob->start();
}

void Settings::setPassword(const QString &password)
{
    if (password == m_password) {
        return;
    }

    if (mapTransportAuthToKimap((MailTransport::TransportBase::EnumAuthenticationType)authentication()) == KIMAP::LoginJob::GSSAPI) {
        return;
    }

    m_password = password;

    auto writePasswordJob = new WritePasswordJob{QStringLiteral("imap"), this};
    writePasswordJob->setKey(config()->name());
    writePasswordJob->setTextData(password);

    connect(writePasswordJob, &WritePasswordJob::finished, this, [this, writePasswordJob](auto) {
        if (writePasswordJob->error() != Error::NoError) {
            handleError(
                i18nc("@info:status", "An error occured when saving the IMAP password in the system keychain: \"%1\"", writePasswordJob->errorString()));
            return;
        }
    });
    writePasswordJob->start();
}

void Settings::loadAccount(ImapAccount *account) const
{
    account->setServer(imapServer());
    if (imapPort() >= 0) {
        account->setPort(imapPort());
    }

    account->setUserName(userName());
    account->setSubscriptionEnabled(subscriptionEnabled());
    account->setUseNetworkProxy(useProxy());

    const QString encryption = safety();
    if (encryption == QLatin1StringView("SSL")) {
        account->setEncryptionMode(KIMAP::LoginJob::SSLorTLS);
    } else if (encryption == QLatin1StringView("STARTTLS")) {
        account->setEncryptionMode(KIMAP::LoginJob::STARTTLS);
    } else {
        account->setEncryptionMode(KIMAP::LoginJob::Unencrypted);
    }

    // Some SSL Server fail to advertise an ssl version they support (AnySslVersion),
    // we therefore allow overriding this in the config
    //(so we don't have to make the UI unnecessarily complex for properly working servers).
    const QString overrideEncryptionMode = overrideEncryption();
    if (!overrideEncryptionMode.isEmpty()) {
        qCWarning(IMAPRESOURCE_LOG) << "Overriding encryption mode with: " << overrideEncryptionMode;
        if (overrideEncryptionMode == QLatin1StringView("SSLV2")) {
            account->setEncryptionMode(KIMAP::LoginJob::SSLorTLS);
        } else if (overrideEncryptionMode == QLatin1StringView("SSLV3")) {
            account->setEncryptionMode(KIMAP::LoginJob::SSLorTLS);
        } else if (overrideEncryptionMode == QLatin1StringView("TLSV1")) {
            account->setEncryptionMode(KIMAP::LoginJob::SSLorTLS);
        } else if (overrideEncryptionMode == QLatin1StringView("SSL")) {
            account->setEncryptionMode(KIMAP::LoginJob::SSLorTLS);
        } else if (overrideEncryptionMode == QLatin1StringView("STARTTLS")) {
            account->setEncryptionMode(KIMAP::LoginJob::STARTTLS);
        } else if (overrideEncryptionMode == QLatin1StringView("UNENCRYPTED")) {
            account->setEncryptionMode(KIMAP::LoginJob::Unencrypted);
        } else {
            qCWarning(IMAPRESOURCE_LOG) << "Tried to force invalid encryption mode: " << overrideEncryptionMode;
        }
    }

    account->setAuthenticationMode(mapTransportAuthToKimap(static_cast<MailTransport::TransportBase::EnumAuthenticationType>(authentication())));

    account->setTimeout(sessionTimeout());
}

QString Settings::rootRemoteId() const
{
    return QStringLiteral("imap://") + userName() + QLatin1Char('@') + imapServer() + QLatin1Char('/');
}

void Settings::renameRootCollection(const QString &newName)
{
    Akonadi::Collection rootCollection;
    rootCollection.setRemoteId(rootRemoteId());
    auto fetchJob = new Akonadi::CollectionFetchJob(rootCollection, Akonadi::CollectionFetchJob::Base);
    fetchJob->setProperty("collectionName", newName);
    connect(fetchJob, &KJob::result, this, &Settings::onRootCollectionFetched);
}

void Settings::onRootCollectionFetched(KJob *job)
{
    const QString newName = job->property("collectionName").toString();
    Q_ASSERT(!newName.isEmpty());
    auto fetchJob = static_cast<Akonadi::CollectionFetchJob *>(job);
    if (fetchJob->collections().size() == 1) {
        Akonadi::Collection rootCollection = fetchJob->collections().at(0);
        rootCollection.setName(newName);
        new Akonadi::CollectionModifyJob(rootCollection);
        // We don't care about the result here, nothing we can/should do if the renaming fails
    }
}

void Settings::handleError(const QString &errorMessage)
{
    auto notification = new KNotification(QStringLiteral("keychain"), KNotification::Persistent);
    notification->setComponentName(QStringLiteral("akonadi_imap_resource"));
    notification->setIconName(QStringLiteral("network-server"));
    notification->setTitle(i18nc("@title", "There was an error interacting with the system keychain"));
    notification->setText(errorMessage);
    notification->sendEvent();
}

#include "moc_settings.cpp"
