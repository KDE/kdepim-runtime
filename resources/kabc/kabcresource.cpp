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

#include <QTimer>

using namespace Akonadi;

using KABC::Resource;

class KABCResource::ErrorHandler : public KABC::ErrorHandler
{
  public:
    explicit ErrorHandler( KABCResource* parent )
      : mParent( parent ) {}

    virtual void error( const QString &message ) {
      mLastError = message;
      emit mParent->error( message );
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
    mExplicitLoading( false ),
    mDelayedUpdateTimer( new QTimer( this ) ),
    mErrorHandler( new ErrorHandler( this ) )
{
  mAddressBook->setErrorHandler( mErrorHandler );

  mDelayedUpdateTimer->setInterval( 10 );
  mDelayedUpdateTimer->setSingleShot( true );

  KSharedConfig::Ptr config = KGlobal::config();
  Q_ASSERT( !config.isNull() );

  KRES::Manager<KABC::Resource> *manager = mAddressBook->getResourceManager();
  manager->readConfig( config.data() );

  mResource = manager->standardResource();
  if ( mResource != 0 ) {
    connect( mResource, SIGNAL( loadingError( Resource*, const QString& ) ),
             this, SLOT(loadingError( Resource*, const QString& ) ) );

    if ( !mResource->isOpen() ) {
      if ( !mResource->open() ) {
        kError() << "Opening resource" << mResource->identifier() << "failed";
      }

    }

    // perform initial load without connected loadingFinished, since it triggers
    // sending items to Akonadi server
    mLoaded = mAddressBook->load();

    connect( mResource, SIGNAL( loadingFinished( Resource* ) ),
             this, SLOT(loadingFinished( Resource* ) ) );
  }

  connect( mAddressBook, SIGNAL( addressBookChanged( AddressBook* ) ),
           this, SLOT( addressBookChanged() ) );

  connect( mDelayedUpdateTimer, SIGNAL( timeout() ),
           this, SLOT( delayedUpdate() ) );
}

KABCResource::~KABCResource()
{
  delete mAddressBook;
}

bool KABCResource::retrieveItem( const Akonadi::Item &item, const QList<QByteArray> &parts )
{
  kDebug() << "item(" << item.id() << "," << item.remoteId() << "),"
           << parts;
  Q_UNUSED( parts );
  const QString rid = item.remoteId();
  KABC::Addressee addressee = mAddressBook->findByUid( item.remoteId() );
  if ( addressee.isEmpty() ) {
    kError() << "Contact with uid" << rid << "not found";
    emit error( i18n( "Contact with uid '%1' not found!", rid ) );
    return false;
  }

  Item i( item );
  i.setMimeType( QLatin1String( "text/directory" ) );
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
    emit error( i18n( "Saving of addressbook failed: could not get Saveticket" ) );
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

  int index = types.indexOf( QLatin1String( "akonadi" ) );
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
  if ( mResource == 0 ) {
    kError() << "Unable to create a KABC resource of type" << type;
    emit error( i18n( "Unable to create a KABC resource of type %1", type ) );
    return;
  }

  mResource->setResourceName( i18n( "%1 address book", type ) );
  mResource->setAddressBook( mAddressBook );

  KRES::ConfigDialog dlg( window, QLatin1String( "contact" ), mResource );

  if ( !dlg.exec() ) {
    delete mResource;
    mResource = 0;
    return;
  }

  manager->writeConfig( KGlobal::config().data() );

  mAddressBook->addResource( mResource );

  connect( mResource, SIGNAL( loadingFinished( Resource* ) ),
           this, SLOT(loadingFinished( Resource* ) ) );
  connect( mResource, SIGNAL( loadingError( Resource*, const QString& ) ),
           this, SLOT(loadingError( Resource*, const QString& ) ) );

  if ( !mResource->isOpen() ) {
    if ( !mResource->open() ) {
      kError() << "Opening resource" << mResource->identifier() << "failed";
    }
  }

  synchronize();
}

void KABCResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& col )
{
  Q_UNUSED( col );

  KABC::Addressee addressee = item.payload<KABC::Addressee>();
  if ( !addressee.isEmpty() ) {
    mAddressBook->insertAddressee( addressee );

    Item i( item );
    i.setRemoteId( addressee.uid() );
    changesCommitted( i );
  } else {
    changeProcessed();
  }
}

void KABCResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& parts )
{
  Q_UNUSED( parts );

  KABC::Addressee addressee = item.payload<KABC::Addressee>();
  if ( !addressee.isEmpty() ) {
    mAddressBook->insertAddressee( addressee );

    Item i( item );
    i.setRemoteId( addressee.uid() );
    changesCommitted( i );
  } else {
    changeProcessed();
  }
}

void KABCResource::itemRemoved( const Akonadi::Item &item )
{
  KABC::Addressee addressee = mAddressBook->findByUid( item.remoteId() );
  if ( !addressee.isEmpty() )
    mAddressBook->removeAddressee( addressee );
  changeProcessed();
}

void KABCResource::retrieveCollections()
{
  kDebug();
  if ( mResource == 0 ) {
    kError() << "No KABC resource";

    emit error( i18n( "No KABC resource plugin configured" ) );

    emit status( Broken, i18n( "No KABC resource plugin configured" ) );
    return;
  }

  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( mResource->identifier() );
  c.setName( name() );

  QStringList mimeTypes;
  mimeTypes << QLatin1String( "text/directory" );
  c.setContentMimeTypes( mimeTypes );

  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void KABCResource::retrieveItems( const Akonadi::Collection &col )
{
  Q_UNUSED( col );

  if ( !mLoaded ) {
    mErrorHandler->mLastError.clear();

    mExplicitLoading = true;
    if ( !mAddressBook->asyncLoad() ) {
      mExplicitLoading = false;
      kError() << "Starting Load failed:" << mErrorHandler->mLastError;
      return;
    }
  } else if ( !mDelayedUpdateTimer->isActive() ) {
    Item::List items;

    AddressBook::const_iterator it    = mAddressBook->begin();
    AddressBook::const_iterator endIt = mAddressBook->end();
    for ( ; it != endIt; ++it ) {
      Item item;
      item.setRemoteId( it->uid() );
      item.setMimeType( QLatin1String( "text/directory" ) );
      items.append( item );
    }

    itemsRetrieved( items );
  }
}

void KABCResource::loadingFinished( KABC::Resource *resource )
{
  Q_UNUSED( resource );

  mLoaded = true;

  if ( mDelayedUpdateTimer->isActive() ) {
    mExplicitLoading = false;
    return;
  }

  if ( mExplicitLoading ) {
    mDelayedUpdateTimer->start();
    mExplicitLoading = false;
  } else
    synchronize();
}

void KABCResource::loadingError( KABC::Resource *resource, const QString &message )
{
  Q_UNUSED( resource );

  kError() << "Loading error: " << message;
  emit error( message );
  emit status( Broken, message );
}

void KABCResource::addressBookChanged()
{
  if ( mExplicitLoading )
    return;

  if ( mDelayedUpdateTimer->isActive() )
    return;

  synchronize();
}

void KABCResource::delayedUpdate()
{
  kDebug();
  Item::List items;

  AddressBook::const_iterator it    = mAddressBook->begin();
  AddressBook::const_iterator endIt = mAddressBook->end();
  for ( ; it != endIt; ++it ) {
    Item item;
    item.setRemoteId( it->uid() );
    item.setMimeType( QLatin1String( "text/directory" ) );
    items.append( item );
  }

  itemsRetrieved( items );
}

AKONADI_RESOURCE_MAIN( KABCResource )

#include "kabcresource.moc"
