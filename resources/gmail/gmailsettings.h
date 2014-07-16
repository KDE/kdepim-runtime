/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef GMAILSETTINGS_H
#define GMAILSETTINGS_H

#include <imap/settings.h>

#include <LibKGAPI2/Types>

namespace KGAPI2 {
class Job;
class AuthJob;
}

class ImapAccount;
class KJob;

class GmailSettings : public Settings
{
  Q_OBJECT

public:
    explicit GmailSettings(WId wid = 0);

    void requestPassword();
    void requestAccount(bool authenticate = false);

    void loadAccount(ImapAccount *account) const;
    void storeAccount(const KGAPI2::AccountPtr &account);

    /* FIXME: I have serious doubts about this methods...they should be in the
     * Resource, not here. */
    QString rootRemoteId() const;

    // Actually cleans tokens
    void clearCachedPassword();
    void cleanup();

    QString apiKey() const;
    QString secretKey() const;

Q_SIGNALS:
    void passwordRequestCompleted(const QString &password, bool userRejected);
    void accountRequestCompleted(const KGAPI2::AccountPtr &account, bool userRejected);

public Q_SLOTS:
    Q_SCRIPTABLE QString accountName(bool *userRejected = 0) const;
    Q_SCRIPTABLE void setAccountName(const QString &accountName);

    Q_SCRIPTABLE QString password(bool *userRejected = 0) const;
    Q_SCRIPTABLE void setPassword(const QString &accessToken);

    Q_SCRIPTABLE QString refreshToken(bool *userRejected = 0) const;
    Q_SCRIPTABLE void setRefreshToken(const QString &refreshToken);

private Q_SLOTS:
    void onWalletOpened(bool success);

    void loadAccountFromKWallet(bool *userRejected = 0) const;
    void saveAccountToKWallet();

    void onAuthFinished(KGAPI2::Job *job);

private:
    mutable KGAPI2::AccountPtr mAccount;
    KGAPI2::AuthJob *mActiveAuthJob;

};

#endif
