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

#include <klocale.h>
#include <kglobal.h>
#include <kwallet.h>
#include <kpassworddialog.h>

using namespace KWallet;

#include <QDBusConnection>

class SettingsHelper
{
  public:
    SettingsHelper() : q( 0 ) {}
    ~SettingsHelper() { delete q; }
    Settings *q;
};

K_GLOBAL_STATIC(SettingsHelper, s_globalSettings)

Settings *Settings::self()
{
  if (!s_globalSettings->q) {
    new Settings;
    s_globalSettings->q->readConfig();
  }

  return s_globalSettings->q;
}

Settings::Settings()
  : SettingsBase(), winId( 0 )
{
  Q_ASSERT( !s_globalSettings->q );
  s_globalSettings->q = this;

  new SettingsAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ), this,
                              QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents );
}

void Settings::setWinId( WId w )
{
  winId = w;
}

QString Settings::password() const
{
  return pwd;
}

void Settings::setPassword( const QString &p )
{
  pwd = p;
}

void Settings::storePassword()
{
  Wallet *wallet = Wallet::openWallet( Wallet::NetworkWallet(), winId );
  
  if( wallet && wallet->isOpen() ) {
    if( !wallet->hasFolder( "dav-akonadi-resource" ) )
      wallet->createFolder( "dav-akonadi-resource" );
    wallet->setFolder( "dav-akonadi-resource" );
    wallet->writePassword( this->username(), pwd );
  }
  
  delete wallet;
}

void Settings::getPassword()
{
  if( this->useKWallet() ) {
    Wallet *wallet = Wallet::openWallet( Wallet::NetworkWallet(), winId );
    
    if( wallet && wallet->isOpen() &&
        wallet->hasFolder( "dav-akonadi-resource" ) &&
        wallet->setFolder( "dav-akonadi-resource" ) ) {
      wallet->readPassword( this->username(), pwd );
    }
    delete wallet;
  }
  else {
    pwd = this->requestPassword( this->username() );
  }
}

QString Settings::requestPassword( const QString &username )
{
  KPasswordDialog dlg;
  dlg.setPrompt( i18n( "Please enter the password for %1" ).arg( username ) );
  if( dlg.exec() )
    pwd = dlg.password();
  return pwd;
}

#include "settings.moc"