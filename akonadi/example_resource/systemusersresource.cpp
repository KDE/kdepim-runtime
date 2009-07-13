/*
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "systemusersresource.h"

#include "settings.h"
#include "settingsadaptor.h"

// we need this for KDE's contact data handling class KABC::Addressee
#include <kabc/addressee.h>

// we need this for using the i18nc() function for translating user visible strings
#include <KLocale>

#include <QtDBus/QDBusConnection>

// we need this two includes for accessing the system's user database
#include <sys/types.h>
#include <pwd.h>

using namespace Akonadi;

SystemUsersResource::SystemUsersResource( const QString &id )
  : ResourceBase( id )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  // this make sure the resource's address book folder and its contacts
  // become available without requring any client interaction
  // Note: a real resource implementation would probably only call
  // synchronizeCollectionTree() and not here but in doSetOnline() or
  // an other method called later on
  synchronize();
}

SystemUsersResource::~SystemUsersResource()
{
}

// this method is called by Akonadi to get the "folders" of our resource
// as a reaction to us calling synchronize() in the constructor
void SystemUsersResource::retrieveCollections()
{
  // create a single collection (visiable as a folder in clients)
  Collection collection;

  // let it be a top level collection, so its parent has to be the root collection
  collection.setParent( Collection::root() );

  // we only have one collection so we can use anything as the remote identifier
  // because we won't have to use it later on for identification purposes.
  // the resource's own identifier is as good as any other non-empty string so
  // we just take it
  collection.setRemoteId( identifier() );

  // we cannot modify the system's user database so we do not allow clients
  // to change anything in our collection
  collection.setRights( Collection::ReadOnly );

  // we tell Akonadi that we will be providing addressbook entries (contacts)
  collection.setContentMimeTypes( QStringList() << KABC::Addressee::mimeType() );

  // this is the string users will see as our folder's name so we need to use
  // one of KDE's translations functions.
  // we use the variant with additional context information so translators
  // know what the purpose of the string is without having to check the code
  collection.setName( i18nc( "@label, name of an addressbook folder", "System Users" ) );

  // finish the collection retrieval by handing our collection back to Akonadi
  Collection::List collections;
  collections << collection;

  collectionsRetrieved( collections );
}

// this method is called by Akonadi to determine how many items a resource
// currently has to a given collection.
// in our case we only have one collection and it contains all system users
void SystemUsersResource::retrieveItems( const Akonadi::Collection &collection )
{
  Q_UNUSED( collection );

  Item::List items;

  // start retrieval from system's user database
  setpwent();

  passwd *pwEnt = 0;
  while ( ( pwEnt = getpwent() ) != 0 ) {
    // we don't need any data here yet, data will be fetched by Akonadi
    // through retrieveItem() (the following method)
    Item item;
    item.setMimeType( KABC::Addressee::mimeType() );

    // to later on know which user the item maps to, we set the user's
    // user id as the item's remote identifier, formatted as a string
    item.setRemoteId( QString::number( pwEnt->pw_uid ) );

    items << item;
  }

  // end retrieval from system's user database
  endpwent();

  // finish by handing our item list back to Akonadi
  itemsRetrieved( items );
}

// this method is called by Akonadi to get data for a specific item.
// some data types can be split into several parts, e.g. emails can
// be split into header, body, attachments, etc.
// we treat contacts as having only one part including everything
bool SystemUsersResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );

  // create a copy so we can modify it (the passed item is const)
  Item newItem( item );

  // get the user's user id from the remote id string. see line 114 above
  uid_t uid = item.remoteId().toUInt();

  // get the system's user database entry for that user
  passwd *pwEnt = getpwuid( uid );

  // convert the POSIX API's data into a KDE addressbook entry
  KABC::Addressee addressee = addresseeFromPWEnt( pwEnt );

  // set the contact as the item's data
  newItem.setPayload<KABC::Addressee>( addressee );

  // finish by handing the full item back to Akonadi
  itemRetrieved( newItem );
  return true;
}

void SystemUsersResource::aboutToQuit()
{
  // don't need to do any cleanup, method could be removed
}

void SystemUsersResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  // don't need any configuration, method could be removed

  // we leave it in as a starting point for experimenting with KConfigXT usage
  // in an Akonadi resource
}

// helper method for converting the system's user database entries into
// KDE addressbook entries
KABC::Addressee SystemUsersResource::addresseeFromPWEnt( passwd *pwEnt )
{
  KABC::Addressee addressee;

  addressee.setNameFromString( QString::fromLocal8Bit( pwEnt->pw_name ) );

  return addressee;
}

AKONADI_RESOURCE_MAIN( SystemUsersResource )

#include "systemusersresource.moc"
