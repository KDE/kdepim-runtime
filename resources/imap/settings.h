/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "settingsbase.h"

#include <KIMAP/LoginJob>

#include <MailTransport/Transport>

class ImapAccount;
class KJob;

namespace QKeychain
{
class ReadPasswordJob;
};

class Settings : public SettingsBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Imap.Wallet")
public:
    enum class Option {
        NoOption = 0,
        ExportToDBus = 1,
    };

    Q_DECLARE_FLAGS(Options, Option)
    static KIMAP::LoginJob::AuthenticationMode mapTransportAuthToKimap(MailTransport::Transport::EnumAuthenticationType authType);

    explicit Settings(WId = 0);
    explicit Settings(const KSharedConfigPtr &config, Options options = Option::ExportToDBus);

    void setWinId(WId);

    /// Call this to decided whether you need to call requestPassword or
    /// password.
    ///
    /// \returns whether the password need to be fetched.
    [[nodiscard]] bool mustFetchPassword() const;

    /// Fetch the password from the system keychain.
    /// \note the returned job is initialized but need to be started.
    [[nodiscard]] QKeychain::ReadPasswordJob *requestPassword();

    /// Return the password if available.
    [[nodiscard]] QString password() const;

    /// Call this to decided whether you need to call requestSievePassword or
    /// sievePassword.
    ///
    /// \returns whether the sieve password need to be fetched.
    [[nodiscard]] bool mustFetchSievePassword() const;

    /// Fetch the sieve password from the system keychain.
    /// \note the returned job is initialized but need to be started.
    [[nodiscard]] QKeychain::ReadPasswordJob *requestSieveCustomPassword();

    /// Return the sieve password if available.
    [[nodiscard]] QString sievePassword() const;

    virtual void loadAccount(ImapAccount *account) const;

    [[nodiscard]] QString rootRemoteId() const;
    virtual void renameRootCollection(const QString &newName);

    void clearCachedPassword();
    void cleanup();

public Q_SLOTS:
    Q_SCRIPTABLE virtual void setPassword(const QString &password);
    Q_SCRIPTABLE virtual void setSieveCustomPassword(const QString &password);

protected Q_SLOTS:
    void onRootCollectionFetched(KJob *job);
    void handleError(const QString &errorMessage);

protected:
    WId m_winId;
    mutable QString m_password;
    mutable QString m_customSievePassword;
};
