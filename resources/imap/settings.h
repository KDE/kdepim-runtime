/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <info@omat.nl>

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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "settingsbase.h"

#include <kimap/loginjob.h>

#include <mailtransport/transport.h>

class ImapAccount;

class Settings : public SettingsBase
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.Imap.Wallet" )
public:
    static KIMAP::LoginJob::AuthenticationMode mapTransportAuthToKimap( MailTransport::Transport::EnumAuthenticationType::type authType );

    explicit Settings( WId = 0 );
    void setWinId( WId );

    virtual void requestPassword();
    virtual void requestManualAuth();

    virtual void loadAccount( ImapAccount *account ) const;

    QString rootRemoteId() const;
    virtual void renameRootCollection( const QString &newName );

    virtual void clearCachedPassword();
    virtual void cleanup();

signals:
    void passwordRequestCompleted( const QString &password, bool userRejected );

public slots:
    Q_SCRIPTABLE virtual QString password( bool *userRejected = 0 ) const;
    Q_SCRIPTABLE virtual void setPassword( const QString &password );
    Q_SCRIPTABLE virtual void setSieveCustomPassword(const QString & password);
    Q_SCRIPTABLE virtual QString sieveCustomPassword( bool *userRejected = 0 ) const;

protected slots:
    virtual void onWalletOpened( bool success );
    virtual void onDialogFinished( int result );

    void onRootCollectionFetched( KJob *job );

protected:
    WId m_winId;
    mutable QString m_password;
    mutable QString m_customSievePassword;
};

#endif
