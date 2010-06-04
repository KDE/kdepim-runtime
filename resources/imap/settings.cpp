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
#include "settingsadaptor.h"

#include <kwallet.h>
using KWallet::Wallet;

#include <kglobal.h>
#include <klocale.h>
#include <kpassworddialog.h>

#include <QDBusConnection>

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

    new SettingsAdaptor( this );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ), this, QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents );
}

void Settings::setWinId( WId winId )
{
    m_winId = winId;
}

void Settings::requestPassword()
{
  if ( !m_password.isEmpty() ) {
    emit passwordRequestCompleted( m_password, false );
  } else {
    Wallet *wallet = Wallet::openWallet( Wallet::NetworkWallet(), m_winId, Wallet::Asynchronous );
    connect( wallet, SIGNAL( walletOpened(bool) ),
             this, SLOT( onWalletOpened(bool) ) );
  }
}

void Settings::onWalletOpened( bool success )
{
  if ( !success ) {
    emit passwordRequestCompleted( QString(), true );
  } else {
    Wallet *wallet = qobject_cast<Wallet*>( sender() );
    if ( wallet && wallet->hasFolder( "imap" ) ) {
        wallet->setFolder( "imap" );
        wallet->readPassword( config()->name(), m_password );
    }
    emit passwordRequestCompleted( m_password, false );
    wallet->deleteLater();
  }
}

void Settings::requestManualAuth()
{
  KPasswordDialog *dlg = new KPasswordDialog( 0 );
  dlg->setPrompt( i18n( "Could not find a valid password for user '%1' on IMAP server '%2', please enter it here.",
                        userName(), imapServer() ) );
  dlg->setAttribute( Qt::WA_DeleteOnClose );
  connect( dlg, SIGNAL(finished(int)), this, SLOT(onDialogFinished(int)) );
  dlg->show();
}

void Settings::onDialogFinished( int result )
{
  if ( result == QDialog::Accepted ) {
    KPasswordDialog *dlg = qobject_cast<KPasswordDialog*>( sender() );
    emit passwordRequestCompleted( dlg->password(), false );
  } else {
    emit passwordRequestCompleted( QString(), true );
  }
}

QString Settings::password(bool *userRejected) const
{
    if ( userRejected != 0 ) {
      *userRejected = false;
    }

    if ( !m_password.isEmpty() )
      return m_password;
    Wallet* wallet = Wallet::openWallet( Wallet::NetworkWallet(), m_winId );
    if ( wallet && wallet->isOpen() ) {
      if ( wallet->hasFolder( "imap" ) ) {
        wallet->setFolder( "imap" );
        wallet->readPassword( config()->name(), m_password );
      } else {
        wallet->createFolder( "imap" );
      }
    } else if ( userRejected != 0 ) {
        *userRejected = true;
    }
    delete wallet;
    return m_password;
}

void Settings::setPassword( const QString & password )
{
    if ( password == m_password )
        return;
    m_password = password;
    Wallet* wallet = Wallet::openWallet( Wallet::NetworkWallet(), m_winId );
    if ( wallet && wallet->isOpen() ) {
        if ( !wallet->hasFolder( "imap" ) )
            wallet->createFolder( "imap" );
        wallet->setFolder( "imap" );
        wallet->writePassword( config()->name(), password );
        kDebug() << "Wallet save: " << wallet->sync() << endl;
    }
}

#include "settings.moc"
