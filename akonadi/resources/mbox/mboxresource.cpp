/*
    Copyright (c) 2009 Bertjan Broeksem <b.broeksema@kdemail.net>

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

#include "mboxresource.h"

#include <akonadi/attributefactory.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/itemfetchscope.h>
#include <boost/shared_ptr.hpp>
#include <kmime/kmime_message.h>
#include <KWindowSystem>
#include <QtDBus/QDBusConnection>

#include "deleteditemsattribute.h"
#include "lockmethodpage.h"
#include "mbox.h"
#include "settingsadaptor.h"
#include "singlefileresourceconfigdialog.h"

using namespace Akonadi;

typedef boost::shared_ptr<KMime::Message> MessagePtr;

static Entity::Id collectionId(const QString &remoteItemId)
{
  // [CollectionId]:[RemoteCollectionId]:[Offset]
  Q_ASSERT(remoteItemId.split(':').size() == 3);
  return remoteItemId.split(':').first().toLongLong();
}

static QString mboxFile(const QString &remoteItemId)
{
  // [CollectionId]:[RemoteCollectionId]:[Offset]
  Q_ASSERT(remoteItemId.split(':').size() == 3);
  return remoteItemId.split(':').at(1);
}

static quint64 itemOffset(const QString &remoteItemId)
{
  // [CollectionId]:[RemoteCollectionId]:[Offset]
  Q_ASSERT(remoteItemId.split(':').size() == 3);
  return remoteItemId.split(':').last().toULongLong();
}

MboxResource::MboxResource( const QString &id )
  : SingleFileResource<Settings>( id )
  , mMBox( 0 )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                              Settings::self(), QDBusConnection::ExportAdaptors );

  QStringList mimeTypes;
  mimeTypes << "message/rfc822";
  setSupportedMimetypes( mimeTypes, "message-rfc822" );

  // Register the list of deleted items as an attribute of the collection.
  AttributeFactory::registerAttribute<DeletedItemsAttribute>();
}

MboxResource::~MboxResource()
{
}

void MboxResource::configure( WId windowId )
{
  SingleFileResourceConfigDialog<Settings> dlg( windowId );
  dlg.addPage( "Lock method", new LockMethodPage() );
  dlg.setCaption( i18n("Select MBox file") );
  if ( dlg.exec() == QDialog::Accepted ) {
    reloadFile();
  }
}

void MboxResource::retrieveCollections()
{
  const QString mboxFile = KUrl(Settings::self()->path()).path();
  MBox mbox(mboxFile);

  QString errMsg;
  if ( !mbox.isValid( errMsg ) ) {
    emit error( errMsg );
    collectionsRetrieved( Collection::List() );
  }

  Collection col;
  col.setParent(Collection::root());
  col.setRemoteId( mboxFile );
  col.setName(name());

  QStringList mimeTypes;
  mimeTypes << "message/rfc822" << Collection::mimeType();
  col.setContentMimeTypes( mimeTypes );

  collectionsRetrieved(Collection::List() << col);
}

void MboxResource::retrieveItems( const Akonadi::Collection &col )
{
  MBox mbox( col.remoteId() );

  if (Settings::self()->lockfileMethod() == Settings::procmail)
    mbox.setLockFile(Settings::self()->lockfile());

  if (!mbox.isValid()) {
    emit error( i18n("Invalid mbox file: %1", col.remoteId() ) );
    itemsRetrieved(Item::List());
    return;
  }

  if (int rc = mbox.open() != 0) { // This can happen for example when the lock fails.
    emit error(i18n("Error while opening mbox file %1: %2", col.remoteId(), rc));
    itemsRetrieved(Item::List());
    return;
  }

  QList<MsgInfo> entryList;
  if (col.hasAttribute<DeletedItemsAttribute>()) {
    DeletedItemsAttribute *attr = col.attribute<DeletedItemsAttribute>();
    entryList = mbox.entryList(attr->deletedItemOffsets());
  } else { // No deleted items (yet)
    entryList = mbox.entryList();
  }

  Item::List items;
  QString colId = QString::number(col.id());
  QString colRid = col.remoteId();
  foreach (const MsgInfo &entry, entryList) {
    // TODO: Use cache policy to see what actualy has to been set as payload.
    //       Currently most views need a minimal amount of information so the
    //       Items get Envelopes as payload.
    KMime::Message *mail = new KMime::Message();
    mail->setHead(KMime::CRLFtoLF(mbox.readEntryHeaders(entry.first)));
    mail->parse();

    Item item;
    item.setRemoteId(colId + ':' + colRid + ':' + QString::number(entry.first));
    item.setMimeType("message/rfc822");
    item.setSize(entry.second);
    item.setPayload(MessagePtr(mail));
    items << item;
  }

  mbox.close(); // Now we have the items, unlock and close the mbox file.

  itemsRetrieved( items );
}

bool MboxResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED(parts);

  MBox mbox(mboxFile(item.remoteId()));

  if(!mbox.isValid())
    return false;

  mbox.open();

  quint64 offset = itemOffset(item.remoteId());
  const QByteArray rawMsg = mbox.readEntry(offset);

  // This doesn't work for first retrieval, when item.size() == 0.
  //Q_ASSERT(rawMsg.size() == item.size());

  KMime::Message *mail = new KMime::Message();
  mail->setContent(KMime::CRLFtoLF(rawMsg));
  mail->parse();

  Item i(item);
  i.setPayload(MessagePtr(mail));
  itemRetrieved(i);
  return true;
}

void MboxResource::aboutToQuit()
{
}

void MboxResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  MBox mbox(collection.remoteId());
  QString errMsg;
  if (Settings::readOnly() || !mbox.isValid(errMsg)) {
    cancelTask(errMsg);
    return;
  }
  // we can only deal with mail
  if (item.mimeType() != "message/rfc822") {
    cancelTask(i18n("Only email messages can be added to the MBox resource."));
    return;
  }

  mbox.open();

  const MessagePtr mail = item.payload<MessagePtr>();
  const QByteArray rawContent = mail->encodedContent();
  const QString rid = QString::number(collection.id()) + ':' + collection.remoteId()
                      + ':' + QString::number(mbox.writeEntry(rawContent));
  Item i(item);
  i.setRemoteId(rid);

  changeCommitted( i );
}

void MboxResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  if ( parts.contains( "PLD:RFC822" ) ) {
    kDebug() << "MboxResource::itemChanged()" << itemOffset( item.remoteId() );
    // Only complete messages can be stored in a MBox file. Because all messages
    // are stored in one single file we do an ItemDelete and an ItemCreate to
    // prevent that whole file must been rewritten.
    CollectionFetchJob *fetchJob =
      new CollectionFetchJob( Collection( collectionId( item.remoteId() ) )
                              , CollectionFetchJob::Base );

    connect( fetchJob, SIGNAL( result( KJob* ) ),
             this, SLOT( onCollectionFetch( KJob* ) ) );

    mCurrentItemDeletions.insert( fetchJob, item );

    fetchJob->start();
    return;
  }

  changeProcessed();
}

void MboxResource::itemRemoved( const Akonadi::Item &item )
{
  CollectionFetchJob *fetchJob =
    new CollectionFetchJob( Collection( collectionId( item.remoteId() ) )
                          , CollectionFetchJob::Base );

  if ( !fetchJob->exec() ) {
    cancelTask( i18n( "Could not fetch the collection: %1" )
                  .arg( fetchJob->errorString() ) );
    return;
  }

  Q_ASSERT( fetchJob->collections().size() == 1 );
  Collection mboxCollection = fetchJob->collections().first();
  DeletedItemsAttribute *attr
    = mboxCollection.attribute<DeletedItemsAttribute>( Akonadi::Entity::AddIfMissing );
  attr->addDeletedItemOffset( itemOffset( item.remoteId() ) );

  CollectionModifyJob *modifyJob = new CollectionModifyJob( mboxCollection );
  if ( !modifyJob->exec() ) {
    cancelTask( modifyJob->errorString() );
    return;
  }

  changeProcessed();
}

bool MboxResource::readFromFile( const QString &fileName )
{
  delete mMBox;
  mMBox = 0;

  mMBox = new MBox( KUrl( fileName ).path(), Settings::self()->readOnly() );

  switch ( Settings::self()->lockfileMethod() ) {
    case Settings::procmail:
      mMBox->setLockType( MBox::ProcmailLockfile );
      mMBox->setLockFile( Settings::self()->lockfile() );
      break;
    case Settings::mutt_dotlock:
      mMBox->setLockType( MBox::MuttDotlock );
      break;
    case Settings::mutt_dotlock_privileged:
      mMBox->setLockType( MBox::MuttDotlockPrivileged );
      break;
    case Settings::kde_lock_file:
      mMBox->setLockType( MBox::KDELockFile );
      mMBox->setLockFile( Settings::self()->lockfile() );
      break;
  }

  return mMBox->isValid();
}

bool MboxResource::writeToFile( const QString &fileName )
{
  Q_UNUSED( fileName );
  // TODO: Change mbox api
  return true;
}

/// Private slots

void MboxResource::onCollectionFetch( KJob *job )
{
  Q_ASSERT( mCurrentItemDeletions.contains( job ) );
  const Item item = mCurrentItemDeletions.take( job );

  if ( job->error() ) {
    cancelTask( job->errorString() );
    job->deleteLater();
    return;
  }

  CollectionFetchJob *fetchJob = dynamic_cast<CollectionFetchJob*>( job );
  Q_ASSERT( fetchJob );
  Q_ASSERT( fetchJob->collections().size() == 1 );

  job->deleteLater();

  Collection mboxCollection = fetchJob->collections().first();
  DeletedItemsAttribute *attr
    = mboxCollection.attribute<DeletedItemsAttribute>( Akonadi::Entity::AddIfMissing );
  attr->addDeletedItemOffset( itemOffset( item.remoteId() ) );

  CollectionModifyJob *modifyJob = new CollectionModifyJob( mboxCollection );
  mCurrentItemDeletions.insert( modifyJob, item );
  connect( modifyJob, SIGNAL( result( KJob* ) ),
           this, SLOT( onCollectionModify( KJob* ) ) );
  modifyJob->start();
}

void MboxResource::onCollectionModify( KJob *job )
{
  Q_ASSERT( mCurrentItemDeletions.contains( job ) );
  const Item item = mCurrentItemDeletions.take( job );

  if ( job->error() ) {
    // Failed to store the offset of a deleted item in the DeletedItemsAttribute
    // of the collection. In this case we shouldn't try to store the modified
    // item.
    cancelTask( i18n( "Failed to update the changed item because the old item "
                      "could not be deleted Reason: %1" ).arg( job->errorString() ) );
    job->deleteLater();
    return;
  }

  Collection c( collectionId( item.remoteId() ) );
  c.setRemoteId( mboxFile( item.remoteId() ) );

  itemAdded( item, c );
}

AKONADI_RESOURCE_MAIN( MboxResource )

#include "mboxresource.moc"
