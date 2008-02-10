/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "kabcresource.h"

#include <kabc/addressbook.h>
#include <kabc/addressee.h>
#include <kabc/errorhandler.h>
#include <kabc/resource.h>

#include <kresources/factory.h>
#include <kresources/configdialog.h>

#include <kconfig.h>
#include <kinputdialog.h>

using namespace Akonadi;

using KABC::Resource;

class KABCResource::ErrorHandler : public KABC::ErrorHandler
{
  public:
    explicit ErrorHandler( KABCResource* parent )
      : mParent( parent ) {}

    virtual void error( const QString &message ) {
      mLastError = message;
      mParent->error( message );
    }

  public:
    KABCResource *mParent;

    QString mLastError;
};

class KABCResource::AddressBook : public KABC::AddressBook
{
  public:
    AddressBook() {}

    KRES::Manager<KABC::Resource> *getResourceManager()
    {
      return resourceManager();
    }
};

KABCResource::KABCResource( const QString &id )
  : ResourceBase( id ),
    mAddressBook( new AddressBook() ),
    mResource( 0 ),
    mLoaded( false ),
    mErrorHandler( new ErrorHandler( this ) )
{
  mAddressBook->setErrorHandler( mErrorHandler );

  KSharedConfig::Ptr config = KGlobal::config();
  Q_ASSERT( !config.isNull() );

  KRES::Manager<KABC::Resource> *manager = mAddressBook->getResourceManager();
  manager->readConfig( config.data() );

  mResource = manager->standardResource();
  if ( mResource != 0 ) {
    connect( mResource, SIGNAL( loadingFinished( Resource* ) ),
             this, SLOT(loadingFinished( Resource* ) ) );
    connect( mResource, SIGNAL( loadingError( Resource*, const QString& ) ),
             this, SLOT(loadingError( Resource*, const QString& ) ) );
  }
}

KABCResource::~KABCResource()
{
  delete mAddressBook;
  delete mErrorHandler;
}

bool KABCResource::retrieveItem( const Akonadi::Item &item, const QStringList &parts )
{
  kDebug() << "item(" << item.reference().id() << "," << item.reference().remoteId() << "),"
           << parts;
  Q_UNUSED( parts );
  const QString rid = item.reference().remoteId();
  KABC::Addressee addressee = mAddressBook->findByUid( item.reference().remoteId() );
  if ( addressee.isEmpty() ) {
    error( i18n( "Contact with uid '%1' not found!", rid ) );
    return false;
  }
  Item i( item );
  i.setMimeType( "text/directory" );
  i.setPayload<KABC::Addressee>( addressee );
  itemRetrieved( i );
  return true;
}

void KABCResource::aboutToQuit()
{
  mErrorHandler->mLastError.clear();

  KABC::Ticket *ticket = mAddressBook->requestSaveTicket();
  if ( ticket == 0 ) {
    kError() << "Could not get save ticket";
    error( i18n( "Saving of addressbook failed: could not get Saveticket" ) );
    return;
  }

  if ( !mAddressBook->save(ticket) ) {
    kError() << "Saving failed: " << mErrorHandler->mLastError;
    return;
  }

  kDebug() << "Saving succeeded";
}

void KABCResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  QWidget *window = 0; // should use windowId somehow

  if ( mResource != 0 ) {
    disconnect( mResource, SIGNAL( loadingFinished( Resource* ) ),
                this, SLOT(loadingFinished( Resource* ) ) );
    disconnect( mResource, SIGNAL( loadingError( Resource*, const QString& ) ),
                this, SLOT(loadingError( Resource*, const QString& ) ) );

    mAddressBook->removeResource( mResource );
    mResource = 0;
    mLoaded = false;
  }

  KRES::Manager<KABC::Resource> *manager = mAddressBook->getResourceManager();

  QStringList types = manager->resourceTypeNames();
  QStringList descs = manager->resourceTypeDescriptions();

  int index = types.indexOf( "akonadi" );
  if ( index != -1 ) {
    types.removeAt( index );
    descs.removeAt( index );
  }

  bool ok = false;
  QString desc = KInputDialog::getItem( i18n( "Create KABC resource" ),
                                        i18n( "Please select type of the new resource:" ),
                                        descs, 0, false, &ok, window );
  if ( !ok )
    return;

  QString type = types[ descs.indexOf( desc ) ];

  // Create new resource
  mResource = manager->createResource( type );
  if ( !mResource ) {
    error( i18n("Unable to create a KABC resource of type %1", type ) );
    return;
  }

  mResource->setResourceName( i18n( "%1 address book", type ) );
  mResource->setAddressBook( mAddressBook );

  KRES::ConfigDialog dlg( window, QString( "contact" ), mResource );

  if ( !dlg.exec() ) {
    delete mResource;
    mResource = 0;
  }

  manager->writeConfig( KGlobal::config().data() );

  mAddressBook->addResource( mResource );

  connect( mResource, SIGNAL( loadingFinished( Resource* ) ),
           this, SLOT(loadingFinished( Resource* ) ) );
  connect( mResource, SIGNAL( loadingError( Resource*, const QString& ) ),
           this, SLOT(loadingError( Resource*, const QString& ) ) );

  synchronize();
}

void KABCResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& col )
{
  Q_UNUSED( col );

  KABC::Addressee addressee = item.payload<KABC::Addressee>();
  if ( !addressee.isEmpty() ) {
    mAddressBook->insertAddressee( addressee );

    DataReference r( item.reference().id(), addressee.uid() );
    changesCommitted( r );
  }
}

void KABCResource::itemChanged( const Akonadi::Item &item, const QStringList& parts )
{
  Q_UNUSED( parts );

  KABC::Addressee addressee = item.payload<KABC::Addressee>();
  if ( !addressee.isEmpty() ) {
    mAddressBook->insertAddressee( addressee );

    DataReference r( item.reference().id(), addressee.uid() );
    changesCommitted( r );
  }
}

void KABCResource::itemRemoved( const Akonadi::DataReference &ref )
{
  KABC::Addressee addressee = mAddressBook->findByUid( ref.remoteId() );
  if ( !addressee.isEmpty() )
    mAddressBook->removeAddressee( addressee );
}

void KABCResource::retrieveCollections()
{
  if ( mResource == 0 ) {
    kError() << "No KABC resource";

    error( i18n( "No KABC resource plugin configured" ) );

    changeStatus( Error, i18n( "No KABC resource plugin configured" ) );
    return;
  }

  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( mResource->identifier() );
  c.setName( name() );

  QStringList mimeTypes;
  mimeTypes << "text/directory";
  c.setContentTypes( mimeTypes );

  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void KABCResource::retrieveItems( const Akonadi::Collection &col, const QStringList &parts )
{
  Q_UNUSED( col );
  Q_UNUSED( parts );
  if ( !mLoaded ) {
    mErrorHandler->mLastError.clear();

    if ( !mAddressBook->asyncLoad() ) {
      kError() << "Starting Load failed:" << mErrorHandler->mLastError;
      return;
    }
  }
}

void KABCResource::loadingFinished( KABC::Resource *resource )
{
  Q_UNUSED( resource );

  mLoaded = true;

  Item::List items;

  AddressBook::const_iterator it    = mAddressBook->begin();
  AddressBook::const_iterator endIt = mAddressBook->end();
  for ( ; it != endIt; ++it ) {
    Item item( DataReference( -1, it->uid() ) );
    item.setMimeType( "text/directory" );
    items.append( item );
  }

  itemsRetrieved( items );
}

void KABCResource::loadingError( KABC::Resource *resource, const QString &message )
{
  Q_UNUSED( resource );

  kError() << "Loading error: " << message;
  error( message );
  changeStatus( Error, message );
}

#include "kabcresource.moc"
