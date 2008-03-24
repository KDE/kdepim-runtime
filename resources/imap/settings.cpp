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

#include "settings.h"


#include <kwallet.h>
using KWallet::Wallet;

#include <kglobal.h>

class SettingsHelper
{
public:
    SettingsHelper() : q( 0 ) {}
    ~SettingsHelper() {
        delete q;
    }
    Settings *q;
};

K_GLOBAL_STATIC( SettingsHelper, s_globalSettings )

Settings *Settings::self()
{
    if ( !s_globalSettings->q ) {
        new Settings;
        s_globalSettings->q->readConfig();
    }

    return s_globalSettings->q;
}

Settings::Settings( WId winId ) : SettingsBase(), m_winId( winId )
{
    Q_ASSERT( !s_globalSettings->q );
    s_globalSettings->q = this;
}

void Settings::setWinId( WId winId )
{
    m_winId = winId;
}

QString Settings::password() const
{
    QString password;
    Wallet* wallet = Wallet::openWallet( Wallet::NetworkWallet(), m_winId );
    if ( wallet && wallet->isOpen() && wallet->hasFolder( "imaplib" ) ) {
        wallet->setFolder( "imaplib" );
        wallet->readPassword( config()->name(), password );
    }
    delete wallet;
    return password;
}

bool Settings::passwordPossible() const
{
    bool possible = true;
    Wallet* wallet = Wallet::openWallet( Wallet::NetworkWallet(), m_winId );
    if ( !wallet ) {
        possible = false;
    }
    delete wallet;
    return possible;
}

void Settings::setPassword( const QString & password )
{
    Wallet* wallet = Wallet::openWallet( Wallet::NetworkWallet(), m_winId );
    if ( wallet && wallet->isOpen() ) {
        if ( !wallet->hasFolder( "imaplib" ) )
            wallet->createFolder( "imaplib" );
        wallet->setFolder( "imaplib" );
        wallet->writePassword( config()->name(), password );
        kDebug() << "Wallet save: " << wallet->sync() << endl;
    }
}

