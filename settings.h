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

class ImapAccount;

class Settings : public SettingsBase
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.Imap.Wallet" )
public:
    Settings( WId = 0 );
    static Settings *self();
    bool passwordPossible() const;
    void setWinId( WId );

    void requestPassword();
    void requestManualAuth();

    void loadAccount( ImapAccount *account ) const;

signals:
    void passwordRequestCompleted( const QString &password, bool userRejected );

public slots:
    Q_SCRIPTABLE QString password( bool *userRejected = 0 ) const;
    Q_SCRIPTABLE void setPassword( const QString &password );

private slots:
    void onWalletOpened( bool success );
    void onDialogFinished( int result );

private:
    WId m_winId;
    mutable QString m_password;
};

#endif
