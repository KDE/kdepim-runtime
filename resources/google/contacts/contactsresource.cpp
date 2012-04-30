/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "contactsresource.h"
#include "settingsdialog.h"
#include "settings.h"

#include <libkgoogle/accessmanager.h>
#include <libkgoogle/auth.h>
#include <libkgoogle/fetchlistjob.h>
#include <libkgoogle/request.h>
#include <libkgoogle/reply.h>
#include <libkgoogle/objects/contact.h>
#include <libkgoogle/services/contacts.h>

#include <QtCore/QStringList>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtCore/QBuffer>

#include <KLocalizedString>
#include <KDebug>
#include <KIO/AccessManager>
#include <Akonadi/Attribute>
#include <Akonadi/AttributeFactory>
#include <Akonadi/CachePolicy>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>

#include <KABC/Addressee>
#include <KABC/Picture>

using namespace Akonadi;
using namespace KGoogle;

#define RootCollection "root"
#define MyContacts     "myContacts"
#define OtherContacts  "otherContacts"

ContactsResource::ContactsResource( const QString &id ):
  ResourceBase( id ),
  m_account( 0 )
{
  /* Register all types we are going to use in this resource */
  qRegisterMetaType< Services::Contacts >( "Contacts" );

  setNeedsNetwork( true );
  setOnline( true );

  Auth *auth = Auth::instance();
  auth->init( "Akonadi Google", Settings::self()->clientId(), Settings::self()->clientSecret() );

  m_gam = new KGoogle::AccessManager();
  m_photoNam = new KIO::Integration::AccessManager( this );

  connect( m_gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(replyReceived(KGoogle::Reply*)) );

  connect( m_gam, SIGNAL(error(KGoogle::Error,QString)),
           this, SLOT(error(KGoogle::Error,QString)) );
  connect( this, SIGNAL(abortRequested()),
           this, SLOT(slotAbortRequested()) );
  connect( this, SIGNAL(reloadConfiguration()),
           this, SLOT(reloadConfig()) );
  connect( m_photoNam, SIGNAL(finished(QNetworkReply*)),
           this, SLOT(photoRequestFinished(QNetworkReply*)) );

  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->fetchCollection( true );

  if ( !Settings::self()->account().isEmpty() ) {
    if ( !getAccount().isNull() ) {
      synchronize();
    }
  }
}

ContactsResource::~ContactsResource()
{
  delete m_photoNam;
  delete m_gam;
}

void ContactsResource::aboutToQuit()
{
  slotAbortRequested();
}

void ContactsResource::abort()
{
  cancelTask( i18n( "Aborted" ) );
}

void ContactsResource::slotAbortRequested()
{
  abort();
}

void ContactsResource::error( Error errCode, const QString &msg )
{
  cancelTask( msg );

  if ( ( errCode == AuthError ) || ( errCode == BackendNotReady ) ) {
    status( Broken, msg );
  }

}

void ContactsResource::configure( WId windowId )
{
  SettingsDialog *settingsDialog = new SettingsDialog( windowId );

  if ( settingsDialog->exec() == KDialog::Accepted ) {
    Q_EMIT configurationDialogAccepted();

    delete settingsDialog;

    if( !getAccount().isNull() ) {
      synchronize();
    }

  } else {

    Q_EMIT configurationDialogRejected();

    delete settingsDialog;
  }
}

void ContactsResource::reloadConfig()
{
  if ( getAccount().isNull() ) {
    return;
  }

  synchronize();
}

Account::Ptr ContactsResource::getAccount()
{
  if ( !m_account.isNull() )
    return m_account;

  Auth *auth = Auth::instance();
  try {
    m_account = auth->getAccount( Settings::self()->account() );
  } catch ( KGoogle::Exception::BaseException &e ) {
    Q_EMIT status( Broken, e.what() );
    return Account::Ptr();
  }

  return m_account;
}


void ContactsResource::retrieveItems( const Akonadi::Collection &collection )
{
  /* Root collection has no items */
  if ( collection.remoteId() == m_collections[RootCollection].remoteId() ) {
    itemsRetrievalDone();
    return;
  }

  ItemFetchJob *fetchJob = new ItemFetchJob( collection, this );
  fetchJob->fetchScope().fetchFullPayload( true );
  fetchJob->setProperty( "Collection", qVariantFromValue( collection ) );
  connect( fetchJob, SIGNAL(finished(KJob*)),
           this, SLOT(initialItemsFetchJobFinished(KJob*)) );

  fetchJob->start();
}

bool ContactsResource::retrieveItem( const Akonadi::Item &item, const QSet< QByteArray >& parts )
{
  Q_UNUSED( parts );

  if ( item.mimeType() != KABC::Addressee::mimeType() )
    return false;

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return true;
  }

  QUrl url( Services::Contacts::fetchContactUrl( account->accountName(), item.remoteId() ) );

  KGoogle::Request *request;
  request = new KGoogle::Request( url, KGoogle::Request::Fetch, "Contacts", account );
  request->setProperty( "Item", QVariant::fromValue( item ) );

  m_gam->sendRequest( request );

  return true;
}

void ContactsResource::retrieveCollections()
{
  Akonadi::EntityDisplayAttribute *attr = new Akonadi::EntityDisplayAttribute();
  attr->setDisplayName( Settings::self()->account() );

  Collection rootCollection;
  rootCollection.setName( identifier() );
  rootCollection.setRemoteId( identifier() );
  rootCollection.setParentCollection( Akonadi::Collection::root() );
  rootCollection.setContentMimeTypes( QStringList() << Collection::mimeType()
                                      << KABC::Addressee::mimeType() );
  rootCollection.addAttribute( attr );
  rootCollection.setRights( Collection::ReadOnly );
  m_collections[RootCollection] = rootCollection;

  Collection::Rights rights = Collection::CanCreateItem |
                              Collection::CanChangeItem |
                              Collection::CanDeleteItem;

  attr = new Akonadi::EntityDisplayAttribute();
  attr->setDisplayName( i18n( "My Contacts" ) );

  Collection myContacts;
  myContacts.setName( i18n( "My Contacts" ) );
  myContacts.setRemoteId( "http://www.google.com/m8/feeds/groups/" + Settings::self()->account() + "/base/6" );
  myContacts.setParentCollection( rootCollection );
  myContacts.setContentMimeTypes( QStringList() << KABC::Addressee::mimeType() );
  myContacts.addAttribute( attr );
  myContacts.setRights( rights );
  m_collections[MyContacts] = myContacts;


  attr = new Akonadi::EntityDisplayAttribute();
  attr->setDisplayName( i18n( "Other Contacts" ) );

  Collection otherContacts;
  otherContacts.setName( i18n( "Other Contacts" ) );
  otherContacts.setRemoteId( OtherContacts );
  otherContacts.setParentCollection( rootCollection );
  otherContacts.setContentMimeTypes( QStringList() << KABC::Addressee::mimeType() );
  otherContacts.addAttribute( attr );
  otherContacts.setRights( rights );
  m_collections[OtherContacts] = otherContacts;

  collectionsRetrieved( m_collections.values() );
}

void ContactsResource::initialItemsFetchJobFinished( KJob *job )
{
  if ( job->error() ) {
    kDebug() << "Initial item fetch failed: " << job->errorString();
    cancelTask( i18n( "Initial item fetching failed" ) );
    return;
  }

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  Collection collection = job->property( "Collection" ).value< Collection >();

  QUrl url = KGoogle::Services::Contacts::fetchAllContactsUrl( account->accountName(), true );

  QString lastSync = collection.remoteRevision();
  if ( !lastSync.isEmpty() ) {
    KDateTime dt;
    dt.setTime_t( lastSync.toInt() );
    lastSync = AccessManager::dateToRFC3339String( dt );

    url.addQueryItem( "updated-min", lastSync );
  }

  Q_EMIT percent( 0 );

  FetchListJob *fetchJob = new FetchListJob( url, "Contacts", account->accountName() );
  fetchJob->setProperty( "Collection", qVariantFromValue( collection ) );
  connect( fetchJob, SIGNAL(finished(KJob*)), this, SLOT(contactListReceived(KJob*)) );
  connect( fetchJob, SIGNAL(percent(KJob*,ulong)), this, SLOT(emitPercent(KJob*,ulong)) );
  fetchJob->start();
}

void ContactsResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  if ( !item.hasPayload< KABC::Addressee >() )
    return;

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  KABC::Addressee addressee = item.payload< KABC::Addressee >();
  Objects::Contact contact( addressee );

  /* If the contact has been moved into My Contacts group then modify the membership */
  if ( collection.remoteId() == m_collections[MyContacts].remoteId() ) {
    contact.addGroup( collection.remoteId() );
  }

  /* If the contact has been moved to Other Contacts then remove all groups */
  if ( collection.remoteId() == m_collections[OtherContacts].remoteId() ) {
    contact.clearGroups();
  }

  Services::Contacts service;
  QByteArray data = service.objectToXML( &contact );
  /* Add XML header and footer */
  data.prepend( "<atom:entry xmlns:atom=\"http://www.w3.org/2005/Atom\" "
                "xmlns:gd=\"http://schemas.google.com/g/2005\" "
                "xmlns:gContact=\"http://schemas.google.com/contact/2008\">"
                "<atom:category scheme=\"http://schemas.google.com/g/2005#kind\" "
                "term=\"http://schemas.google.com/contact/2008#contact\"/>" );
  data.append( "</atom:entry>" );

  KGoogle::Request *request;
  request = new KGoogle::Request( Services::Contacts::createContactUrl( account->accountName() ),
                                  Request::Create, "Contacts", account );
  request->setRequestData( data, "application/atom+xml" );
  request->setProperty( "Item", QVariant::fromValue( item ) );

  m_gam->sendRequest( request );

  Q_UNUSED( collection );
}

void ContactsResource::itemChanged( const Akonadi::Item &item, const QSet< QByteArray >& partIdentifiers )
{
  if ( !item.hasPayload< KABC::Addressee >() ) {
    cancelTask( i18n( "Invalid Payload" ) );
    return;
  }

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  KABC::Addressee addressee = item.payload< KABC::Addressee >();
  Objects::Contact contact( addressee );

  if ( item.parentCollection().remoteId() == m_collections[MyContacts].remoteId() ) {
    contact.addGroup( m_collections[MyContacts].remoteId() );
  }

  Services::Contacts service;
  QByteArray data = service.objectToXML( &contact );
  /* Add XML header and footer */
  data.prepend( "<atom:entry xmlns:atom=\"http://www.w3.org/2005/Atom\" "
                "xmlns:gd=\"http://schemas.google.com/g/2005\" "
                "xmlns:gContact=\"http://schemas.google.com/contact/2008\">"
                "<atom:category scheme=\"http://schemas.google.com/g/2005#kind\" "
                "term=\"http://schemas.google.com/contact/2008#contact\"/>" );
  data.append( "</atom:entry>" );

  KGoogle::Request *request;
  request = new KGoogle::Request( Services::Contacts::updateContactUrl( account->accountName(), item.remoteId() ),
                                  Request::Update, "Contacts", account );
  request->setRequestData( data, "application/atom+xml" );
  request->setProperty( "Item", QVariant::fromValue( item ) );

  m_gam->sendRequest( request );

  Q_UNUSED( partIdentifiers );
}

void ContactsResource::itemMoved( const Item &item, const Collection &collectionSource, const Collection &collectionDestination )
{
  if ( !item.hasPayload< KABC::Addressee >() ) {
    cancelTask( i18n( "Invalid payload" ) );
    return;
  }

  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  kDebug() << "Moving contact" << item.remoteId() << "from" << collectionSource.remoteId() << "to" << collectionDestination.remoteId();

  KABC::Addressee addressee = item.payload< KABC::Addressee >();
  Objects::Contact contact( addressee );

  if ( collectionSource.remoteId() == m_collections[MyContacts].remoteId()
      && collectionDestination.remoteId() == m_collections[OtherContacts].remoteId() ) {
    contact.removeGroup( collectionSource.remoteId() );
  } else if ( collectionSource.remoteId() == m_collections[OtherContacts].remoteId()
             && collectionDestination.remoteId() == m_collections[MyContacts].remoteId() ) {
    contact.addGroup( collectionDestination.remoteId() );
  } else {
    cancelTask( i18n( "Invalid source or destination collection" ) );
    return;
  }

  Services::Contacts service;
  QByteArray data = service.objectToXML( &contact );
  /* Add XML header and footer */
  data.prepend( "<atom:entry xmlns:atom=\"http://www.w3.org/2005/Atom\" "
                "xmlns:gd=\"http://schemas.google.com/g/2005\" "
                "xmlns:gContact=\"http://schemas.google.com/contact/2008\">"
                "<atom:category scheme=\"http://schemas.google.com/g/2005#kind\" "
                "term=\"http://schemas.google.com/contact/2008#contact\"/>" );
  data.append( "</atom:entry>" );

  KGoogle::Request *request;
  request = new KGoogle::Request( Services::Contacts::updateContactUrl( account->accountName(), item.remoteId() ),
                                  Request::Update, "Contacts", account );
  request->setRequestData( data, "application/atom+xml" );
  request->setProperty( "Item", QVariant::fromValue( item ) );

  m_gam->sendRequest( request );
}


void ContactsResource::itemRemoved( const Akonadi::Item &item )
{
  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  KGoogle::Request *request;
  request = new KGoogle::Request( Services::Contacts::removeContactUrl( account->accountName(), item.remoteId() ),
                                  Request::Remove, "Contacts", account );
  request->setProperty( "Item", QVariant::fromValue( item ) );

  m_gam->sendRequest( request );
}


void ContactsResource::replyReceived( KGoogle::Reply *reply )
{
  switch ( reply->requestType() ) {
    case Request::Fetch:
      contactReceived( reply );
      break;

    case Request::Create:
      contactCreated( reply );
      break;

    case Request::Update:
      contactUpdated( reply );
      break;

    case Request::Remove:
      contactRemoved( reply );
      break;

    case Request::FetchAll:
    case Request::Move:
    case Request::Patch:
      break;
  }

  delete reply;
}

void ContactsResource::contactListReceived( KJob *job )
{
  if ( job->error() ) {
    cancelTask( i18n( "Failed to retrieve contacts" ) );
    return;
  }

  Collection collection = job->property( "Collection" ).value<Collection>();

  Item::List removed;
  Item::List changed;

  FetchListJob *fetchJob = dynamic_cast< FetchListJob * >( job );
  QList< KGoogle::Object * > objects = fetchJob->items();

  Q_FOREACH ( KGoogle::Object * object, objects ) {

    Item item;
    Objects::Contact *contact = static_cast< Objects::Contact * >( object );

    /* If we are fetching "Other Contacts" and the contact does not have
     * empty member ship the skip it. For the "My Contacts" collection skip
     * all contacts with empty groups. */
    if ( collection.remoteId() == m_collections[OtherContacts].remoteId() ) {
      if ( !contact->groups().isEmpty() ) {
        continue;
      }
    } else {
      if ( contact->groups().isEmpty() ) {
        continue;
      }
    }

    item.setRemoteId( contact->uid() );

    if ( contact->deleted() ) {
      removed << item;
    } else {

      item.setRemoteRevision( contact->etag() );
      item.setMimeType( contact->mimeType() );
      item.setPayload< KABC::Addressee >( KABC::Addressee( *contact ) );

      changed << item;

      fetchPhoto( item );
    }
  }

  itemsRetrievedIncremental( changed, removed );

  collection.setRemoteRevision( QString::number( KDateTime::currentUtcDateTime().toTime_t() ) );
  CollectionModifyJob *modifyJob = new CollectionModifyJob( collection, this );
  modifyJob->setAutoDelete( true );
}

void ContactsResource::contactReceived( KGoogle::Reply *reply )
{
  if ( reply->error() != KGoogle::OK ) {
    cancelTask( i18n( "Failed to fetch contact" ) );
    return;
  }

  QList< KGoogle::Object * > data = reply->replyData();
  if ( data.length() != 1 ) {
    kWarning() << "Server send " << data.length() << "items, which is not OK";
    cancelTask( i18n( "Failed to create a contact" ) );
    return;
  }

  Objects::Contact *contact = static_cast< Objects::Contact * >( data.first() );

  Item item = reply->request()->property( "Item" ).value< Item >();
  item.setRemoteId( contact->uid() );
  item.setRemoteRevision( contact->etag() );

  if ( contact->deleted() ) {
    itemsRetrievedIncremental( Item::List(), Item::List() << item );
  } else {
    item.setPayload< KABC::Addressee >( KABC::Addressee( *contact ) );
    item.setMimeType( contact->mimeType() );

    itemsRetrieved( Item::List() << item );

    fetchPhoto( item );
  }
}

void ContactsResource::contactCreated( KGoogle::Reply *reply )
{
  if ( reply->error() != KGoogle::Created ) {
    cancelTask( i18n( "Failed to create a contact" ) );
    return;
  }

  QList< KGoogle::Object * > data = reply->replyData();
  if ( data.length() != 1 ) {
    kWarning() << "Server send " << data.length() << "items, which is not OK";
    cancelTask( i18n( "Failed to create a contact" ) );
    return;
  }

  Objects::Contact *contact = ( Objects::Contact * ) data.first();

  Item item = reply->request()->property( "Item" ).value< Item >();
  item.setRemoteId( contact->uid() );
  item.setRemoteRevision( contact->etag() );

  changeCommitted( item );

  item.setPayload< KABC::Addressee >( KABC::Addressee( *contact ) );
  ItemModifyJob *modifyJob = new ItemModifyJob( item );
  modifyJob->setAutoDelete( true );

  updatePhoto( item );
}

void ContactsResource::contactUpdated( KGoogle::Reply *reply )
{
  if ( reply->error() != KGoogle::OK ) {
    cancelTask( i18n( "Failed to update contact" ) );
    return;
  }

  QList< KGoogle::Object * > data = reply->replyData();
  if ( data.length() != 1 ) {
    kWarning() << "Server send " << data.length() << "items, which is not OK";
    cancelTask( i18n( "Failed to update a contact" ) );
    return;
  }

  Objects::Contact *contact = static_cast< Objects::Contact * >( data.first() );

  Item item = reply->request()->property( "Item" ).value< Item >();
  item.setRemoteId( contact->uid() );
  item.setRemoteRevision( contact->etag() );

  changeCommitted( item );

  updatePhoto( item );
}

void ContactsResource::contactRemoved( KGoogle::Reply *reply )
{
  if ( reply->error() != KGoogle::OK ) {
    cancelTask( i18n( "Failed to remove contact" ) );
    return;
  }

  Item item = reply->request()->property( "Item" ).value< Item >();

  changeCommitted( item );
}

void ContactsResource::photoRequestFinished( QNetworkReply *reply )
{
  /* We care only about retrieving a contact's photo. */
  if ( reply->operation() == QNetworkAccessManager::GetOperation ) {
    QImage image;

    if ( !image.loadFromData( reply->readAll(), "JPG" ) )
      return;


    Item item = reply->request().attribute( QNetworkRequest::User, QVariant() ).value< Item >();

    KABC::Addressee addressee = item.payload< KABC::Addressee >();
    addressee.setPhoto( KABC::Picture( image ) );
    item.setPayload< KABC::Addressee >( addressee );

    ItemModifyJob *modifyJob = new ItemModifyJob( item );
    modifyJob->setAutoDelete( true );
  }
}

void ContactsResource::fetchPhoto( Akonadi::Item &item )
{
  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  QString id = item.remoteId().mid( item.remoteId().lastIndexOf( "/" ) );

  QNetworkRequest request;
  request.setUrl( Services::Contacts::photoUrl( account->accountName(), id ) );
  request.setRawHeader( "Authorization", "OAuth " + account->accessToken().toLatin1() );
  request.setRawHeader( "GData-Version", "3.0" );

  request.setAttribute( QNetworkRequest::User, qVariantFromValue( item ) );
  m_photoNam->get( request );
}


void ContactsResource::updatePhoto( Item &item )
{
  Account::Ptr account = getAccount();
  if ( account.isNull() ) {
    deferTask();
    return;
  }

  QString id = item.remoteId().mid( item.remoteId().lastIndexOf( "/" ) );

  KABC::Addressee addressee = item.payload< KABC::Addressee >();
  QNetworkRequest request;
  request.setUrl( Services::Contacts::photoUrl( account->accountName(), id ) );
  request.setRawHeader( "Authorization", "OAuth " + account->accessToken().toLatin1() );
  request.setRawHeader( "GData-Version", "3.0" );
  request.setRawHeader( "If-Match", "*" );

  if ( !addressee.photo().isEmpty() ) {
    request.setHeader( QNetworkRequest::ContentTypeHeader, "image/*" );

    QImage image = addressee.photo().data();
    QByteArray ba;
    QBuffer buffer( &ba );
    image.save( &buffer, "JPG", 100 );

    m_photoNam->put( request, ba );
  } else {
    m_photoNam->deleteResource( request );
  }
}

void ContactsResource::emitPercent( KJob *job, ulong progress )
{
  Q_EMIT percent( progress );

  Q_UNUSED( job )
}


AKONADI_RESOURCE_MAIN( ContactsResource )
