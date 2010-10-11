/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "kdeaccountsresource.h"

#include "settingsadaptor.h"
#include "singlefileresourceconfigdialog.h"

#include <akonadi/dbusconnectionpool.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <KWindowSystem>


using namespace Akonadi;

KDEAccountsResource::KDEAccountsResource( const QString &id )
  : SingleFileResource<Settings>( id )
{
  setSupportedMimetypes( QStringList() << KABC::Addressee::mimeType(), "kde" );

  setName( i18n( "KDE Accounts" ) );

  new SettingsAdaptor( mSettings );
  DBusConnectionPool::threadConnection().registerObject( QLatin1String( "/Settings" ),
                                                         mSettings, QDBusConnection::ExportAdaptors );
}

KDEAccountsResource::~KDEAccountsResource()
{
  mContacts.clear();
}

bool KDEAccountsResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  const QString remoteId = item.remoteId();
  if ( !mContacts.contains( remoteId ) ) {
    cancelTask( i18n( "Contact with uid '%1' not found.", remoteId ) );
    return false;
  }

  Item newItem( item );
  newItem.setPayload<KABC::Addressee>( mContacts.value( remoteId ) );
  itemRetrieved( newItem );

  return true;
}

void KDEAccountsResource::customizeConfigDialog( SingleFileResourceConfigDialog<Settings>* dlg )
{
  dlg->setCaption( i18n( "Select KDE Accounts File" ) );
}

void KDEAccountsResource::configDialogAcceptedActions( SingleFileResourceConfigDialog<Settings>* )
{
    // We can't hide the GUI element but we can enforce that the
    // resource is read-only
    mSettings->setReadOnly( true );
    mSettings->writeConfig();
}

void KDEAccountsResource::itemAdded( const Akonadi::Item&, const Akonadi::Collection& )
{
  cancelTask( i18n( "KDE Accounts file is read-only" ) );
}

void KDEAccountsResource::itemChanged( const Akonadi::Item&, const QSet<QByteArray>& )
{
  cancelTask( i18n( "KDE Accounts file is read-only" ) );
}

void KDEAccountsResource::itemRemoved( const Akonadi::Item& )
{
  cancelTask( i18n( "KDE Accounts file is read-only" ) );
}

void KDEAccountsResource::retrieveItems( const Akonadi::Collection &collection )
{
  // KDEAccounts does not support folders so we can safely ignore the collection
  Q_UNUSED( collection );

  Akonadi::Item::List items;

  foreach ( const KABC::Addressee &contact, mContacts ) {
    Item item;
    item.setRemoteId( contact.uid() );
    item.setMimeType( KABC::Addressee::mimeType() );
    item.setPayload( contact );
    items.append( item );
  }

  itemsRetrieved( items );
}

bool KDEAccountsResource::readFromFile( const QString &fileName )
{
  mContacts.clear();

  QFile file( KUrl( fileName ).toLocalFile() );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    emit status( Broken, i18n( "Unable to open KDE accounts file '%1'.", fileName ) );
    return false;
  }

  while ( !file.atEnd() ) {
    QString line = QString::fromUtf8( file.readLine() );
    // We have a line like the following now, which consists of a nickname,
    // the full name and an email address
    // 'tokoe     Tobias Koenig     tokoe@kde.org'

    // remove all leading, trailing and duplicated white spaces
    line = line.trimmed().simplified();

    QStringList parts = line.split( QLatin1Char( ' ' ) );

    // The first part is the nick name, the last one the email address and
    // everything else in between the full name
    const QString nickName = parts.takeFirst();
    const QString email = parts.takeLast();
    const QString fullName = parts.join( QLatin1String( " " ) );

    KABC::Addressee contact;
    contact.setNickName( nickName  );
    contact.setNameFromString( fullName );
    contact.setOrganization( i18n( "KDE Project" ) );
    contact.insertCategory( i18n( "KDE Developer" ) );
    contact.insertEmail( email  );

    mContacts.insert( contact.uid(), contact );
  }

  return true;
}

bool KDEAccountsResource::writeToFile( const QString& )
{
  Q_ASSERT( false ); // this method should never be called

  return false;
}

AKONADI_RESOURCE_MAIN( KDEAccountsResource )

#include "kdeaccountsresource.moc"
