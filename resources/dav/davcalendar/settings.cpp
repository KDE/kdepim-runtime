/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>
      Based on an original work for the IMAP resource which is :
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <info@omat.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "settings.h"

#include "settingsadaptor.h"

#include <kglobal.h>
#include <klocale.h>
#include <kpassworddialog.h>
#include <kwallet.h>

#include <QtDBus/QDBusConnection>

using namespace KWallet;

class SettingsHelper
{
  public:
    SettingsHelper()
      : q( 0 )
    {
    }

    ~SettingsHelper()
    {
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

Settings::Settings()
  : SettingsBase(), mWinId( 0 ), mWallet( 0 )
{
  Q_ASSERT( !s_globalSettings->q );
  s_globalSettings->q = this;

  new SettingsAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ), this,
                              QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents );
}

Settings::~Settings()
{
  delete mWallet;
}

void Settings::setWinId( WId winId )
{
  mWinId = winId;
}

QString Settings::password() const
{
  return mPassword;
}

void Settings::setPassword( const QString &password )
{
  mPassword = password;

  if ( useKWallet() )
    storePassword();
}

void Settings::storePassword()
{
  if ( !mWallet )
    mWallet = Wallet::openWallet( Wallet::NetworkWallet(), mWinId );

  if ( mWallet && mWallet->isOpen() ) {
    if ( !mWallet->hasFolder( "dav-akonadi-resource" ) )
      mWallet->createFolder( "dav-akonadi-resource" );

    mWallet->setFolder( "dav-akonadi-resource" );
    mWallet->writePassword( username(), mPassword );
  }
}

void Settings::askForPassword()
{
  if ( !password().isEmpty() )
    return;

  if ( useKWallet() ) {
    if ( !mWallet )
      mWallet = Wallet::openWallet( Wallet::NetworkWallet(), mWinId );

    if ( mWallet && mWallet->isOpen() &&
         mWallet->hasFolder( "dav-akonadi-resource" ) &&
         mWallet->setFolder( "dav-akonadi-resource" ) ) {
      mWallet->readPassword( username(), mPassword );
    }
  }

  if ( mPassword.isEmpty() )
    requestPassword( username() );
}

void Settings::requestPassword( const QString &username )
{
  KPasswordDialog dlg;
  dlg.setPrompt( i18n( "Please enter the password for %1" ).arg( username ) );
  dlg.exec();

  if ( dlg.result() == QDialog::Accepted )
    setPassword( dlg.password() );
}

#include "settings.moc"
