// -*- c-basic-offset: 2 -*-
/*
    This file is part of libkabc.
    Copyright (c) 2008-2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#include "resourceakonadi.h"
#include "resourceakonadiconfig.h"

#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/collectionmodel.h>
#include <akonadi/control.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/monitor.h>
#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/transactionsequence.h>

#include <kabc/contactgroup.h>

#include <kconfiggroup.h>
#include <kdebug.h>

#include <QtConcurrentRun>
#include <QFuture>
#include <QHash>

using namespace Akonadi;
using namespace KABC;

class UidToCollectionMapper
{
public:
  virtual ~UidToCollectionMapper() {}

  virtual Collection collectionForUid( const QString &uid ) const = 0;
};

static KJob *createSaveSequence( const UidToCollectionMapper &mapper,
    const Item::List &addedItems, const Item::List &modifiedItems, const Item::List &deletedItems );

class ThreadJobContext
{
  public:
    explicit ThreadJobContext( const UidToCollectionMapper &mapper )
      : mMapper( mapper ) {}

    void clear()
    {
      mJobError.clear();
      mCollections.clear();
      mItems.clear();

      mAddedItems.clear();
      mModifiedItems.clear();
      mDeletedItems.clear();
    }

    bool fetchCollections()
    {
      CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );

      bool jobResult = job->exec();
      if ( jobResult )
        mCollections = job->collections();
      else
        mJobError = job->errorString();

      delete job;
      return jobResult;
    }

    bool fetchItems( const Collection &collection )
    {
      ItemFetchJob *job = new ItemFetchJob( collection );
      job->fetchScope().fetchFullPayload();

      bool jobResult = job->exec();
      if ( jobResult )
        mItems = job->items();
      else
        mJobError = job->errorString();

      delete job;
      return jobResult;
    }

    bool saveChanges()
    {
      KJob *job = createSaveSequence( mMapper, mAddedItems, mModifiedItems, mDeletedItems );

      bool jobResult = job->exec();
      if ( !jobResult )
        mJobError = job->errorString();

      delete job;
      return jobResult;
    }

  public:
    QString mJobError;

    Collection::List mCollections;
    Item::List mItems;

    Item::List mAddedItems;
    Item::List mModifiedItems;
    Item::List mDeletedItems;

  private:
    const UidToCollectionMapper &mMapper;

  private:
};

typedef QMap<Item::Id, Item> ItemMap;
typedef QHash<QString, Item::Id> IdHash;

class SubResource
{
  public:
    SubResource( const Collection &collection, const KConfigGroup &parentGroup )
        : mCollection( collection ), mLabel( collection.name() ),
          mActive(true), mCompletionWeight(80)
    {
      if ( mCollection.hasAttribute<EntityDisplayAttribute>() &&
           !mCollection.attribute<EntityDisplayAttribute>()->displayName().isEmpty() )
        mLabel = mCollection.attribute<EntityDisplayAttribute>()->displayName();

      readConfig( parentGroup );
    }

    void setActive( bool active )
    {
      mActive = active;
    }

    bool isActive() const { return mActive; }

    bool isWritable() const { return mCollection.rights() != Collection::ReadOnly; }

    void setCompletionWeight( int weight )
    {
      mCompletionWeight = weight;
    }

    int completionWeight() const { return mCompletionWeight; }

    void writeConfig( KConfigGroup &parentGroup ) const
    {
      KConfigGroup group( &parentGroup, mCollection.url().url() );

      group.writeEntry( QLatin1String( "Active" ), mActive );
      group.writeEntry( QLatin1String( "CompletionWeight" ), mCompletionWeight );
    }

    void readConfig( const KConfigGroup &parentGroup )
    {
      if ( !parentGroup.isValid() )
        return;

      const QString collectionUrl = mCollection.url().url();
      if ( !parentGroup.hasGroup( collectionUrl ) )
        return;

      KConfigGroup group( &parentGroup, collectionUrl );
      mActive = group.readEntry<bool>( QLatin1String( "Active" ), true );
      mCompletionWeight = group.readEntry<int>( QLatin1String( "CompletionWeight" ), 80 );
    }

    Collection collection() const
    {
      return mCollection;
    }

  public:
    Collection mCollection;
    QString mLabel;

    bool mActive;
    int  mCompletionWeight;
};

typedef QHash<QString, SubResource*> SubResourceMap;

typedef QMap<QString, QString>  UidResourceMap;
typedef QMap<Item::Id, QString> ItemIdResourceMap;

class ResourceAkonadi::Private : public UidToCollectionMapper
{
  public:
    Private( ResourceAkonadi *parent )
      : mParent( parent ), mMonitor( 0 ), mCollectionModel( 0 ), mCollectionFilterModel( 0 ),
        mThreadJobContext( *this )
    {
    }

    Collection collectionForUid( const QString &uid ) const
    {
      SubResource *subResource = mSubResources.value( mUidToResourceMap.value( uid ), 0 );
      Q_ASSERT( subResource != 0 );

      return subResource->mCollection;
    }

 public:
    ResourceAkonadi *mParent;

    KConfigGroup mConfig;

    Monitor *mMonitor;

    ItemMap mItems;
    IdHash  mIdMapping;

    Collection mStoreCollection;

    CollectionModel *mCollectionModel;
    CollectionFilterProxyModel *mCollectionFilterModel;

    SubResourceMap mSubResources;
    QSet<QString> mSubResourceIds;

    UidResourceMap    mUidToResourceMap;
    ItemIdResourceMap mItemIdToResourceMap;

    QHash<Akonadi::Job*, QString> mJobToResourceMap;

    enum ChangeType
    {
      Added,
      Changed,
      Removed
    };

    typedef QMap<QString, ChangeType> ChangeMap;
    ChangeMap mChanges;

    QSet<QString> mAsyncLoadCollections;

    ThreadJobContext mThreadJobContext;

  public:
    void subResourceLoadResult( KJob *job );

    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers );
    void itemRemoved( const Akonadi::Item &item );

    void collectionRowsInserted( const QModelIndex &parent, int start, int end );
    void collectionRowsRemoved( const QModelIndex &parent, int start, int end );
    void collectionDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight );

    void addCollectionsRecursively( const QModelIndex &parent, int start, int end );
    bool removeCollectionsRecursively( const QModelIndex &parent, int start, int end );

    Collection findDefaultCollection() const;
    bool prepareSaving();
    KJob *createSaveSequence() const;

    void createItemListsForSave( Item::List &added, Item::List &modified, Item::List &deleted ) const;

    bool reloadSubResource( SubResource *subResource, bool &changed );

    DistributionList *distListFromContactGroup( const ContactGroup &contactGroup );

    ContactGroup contactGroupFromDistList( const DistributionList* list ) const;
};

ResourceAkonadi::ResourceAkonadi()
  : ResourceABC(), d( new Private( this ) )
{
  init();
}

ResourceAkonadi::ResourceAkonadi( const KConfigGroup &group )
  : ResourceABC( group ), d( new Private( this ) )
{
  KUrl url = group.readEntry( QLatin1String( "CollectionUrl" ), KUrl() );

  if ( url.isValid() )
    d->mStoreCollection = Collection::fromUrl( url );

  d->mConfig = group;

  init();
}

ResourceAkonadi::~ResourceAkonadi()
{
  delete d;
}

void ResourceAkonadi::init()
{
  // TODO: might be better to do this already in the resource factory
  Akonadi::Control::start();
}

void ResourceAkonadi::clear()
{
  // clear local caches
  d->mItems.clear();
  d->mIdMapping.clear();

  qDeleteAll( d->mSubResources );
  d->mSubResources.clear();
  d->mSubResourceIds.clear();

  d->mChanges.clear();

  d->mAsyncLoadCollections.clear();

  ResourceABC::clear();
}

void ResourceAkonadi::writeConfig( KConfigGroup &group )
{
  ResourceABC::writeConfig( group );

  group.writeEntry( QLatin1String( "CollectionUrl" ), d->mStoreCollection.url() );

  SubResourceMap::const_iterator it    = d->mSubResources.constBegin();
  SubResourceMap::const_iterator endIt = d->mSubResources.constEnd();
  for (; it != endIt; ++it ) {
    it.value()->writeConfig( group );
  }

  d->mConfig = group;
}

bool ResourceAkonadi::doOpen()
{
  // if already connected, ignore
  if ( d->mCollectionFilterModel != 0 )
    return true;

  // TODO: probably check if Akonadi is running

  d->mCollectionModel = new CollectionModel( this );

  d->mCollectionFilterModel = new CollectionFilterProxyModel( this );
  d->mCollectionFilterModel->addMimeTypeFilter( QLatin1String( "text/directory" ) );
  d->mCollectionFilterModel->addMimeTypeFilter( ContactGroup::mimeType() );

  connect( d->mCollectionFilterModel, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
           this, SLOT( collectionRowsInserted( const QModelIndex&, int, int ) ) );
  connect( d->mCollectionFilterModel, SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ),
           this, SLOT( collectionRowsRemoved( const QModelIndex&, int, int ) ) );
  connect( d->mCollectionFilterModel, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( collectionDataChanged( const QModelIndex&, const QModelIndex& ) ) );

  d->mCollectionFilterModel->setSourceModel( d->mCollectionModel );

  d->mMonitor = new Monitor( this );

  d->mMonitor->setMimeTypeMonitored( QLatin1String( "text/directory" ) );
  d->mMonitor->setMimeTypeMonitored( ContactGroup::mimeType() );
  d->mMonitor->itemFetchScope().fetchFullPayload();

  connect( d->mMonitor,
           SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
           this,
           SLOT( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );

  connect( d->mMonitor,
           SIGNAL( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ),
           this,
           SLOT( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ) );

  connect( d->mMonitor,
           SIGNAL( itemRemoved( const Akonadi::Item& ) ),
           this,
           SLOT( itemRemoved( const Akonadi::Item& ) ) );

  return true;
}

void ResourceAkonadi::doClose()
{
  delete d->mMonitor;
  d->mMonitor = 0;
  delete d->mCollectionFilterModel;
  d->mCollectionFilterModel = 0;
  delete d->mCollectionModel;
  d->mCollectionModel = 0;

  // clear local caches
  mAddrMap.clear();
  qDeleteAll( mDistListMap );
  mDistListMap.clear();
  d->mItems.clear();
  d->mIdMapping.clear();
  d->mUidToResourceMap.clear();
  d->mItemIdToResourceMap.clear();
  d->mJobToResourceMap.clear();
  d->mChanges.clear();
}

Ticket *ResourceAkonadi::requestSaveTicket()
{
  if ( !addressBook() ) {
    kDebug(5700) << "no addressbook";
    return 0;
  }

  return createTicket( this );
}

void ResourceAkonadi::releaseSaveTicket( Ticket *ticket )
{
  delete ticket;
}

bool ResourceAkonadi::load()
{
  kDebug(5700);

  // save the list of collections we potentially already have
  Collection::List collections;
  SubResourceMap::const_iterator it    = d->mSubResources.constBegin();
  SubResourceMap::const_iterator endIt = d->mSubResources.constEnd();
  for ( ; it != endIt; ++it ) {
    collections << it.value()->mCollection;
  }

  clear();

  // if we do not have any collection yet, fetch them explicitly
  if ( collections.isEmpty() ) {
    d->mThreadJobContext.clear();
    QFuture<bool> threadResult =
      QtConcurrent::run( &(d->mThreadJobContext), &ThreadJobContext::fetchCollections );
    if ( !threadResult.result() ) {
      emit loadingError( this, d->mThreadJobContext.mJobError );
      return false;
    }

    collections = d->mThreadJobContext.mCollections;
  }

  bool result = true;
  foreach ( const Collection &collection, collections ) {
     if ( !collection.contentMimeTypes().contains( QLatin1String( "text/directory" ) )
          && !collection.contentMimeTypes().contains( ContactGroup::mimeType() ) )
       continue;

    const QString collectionUrl = collection.url().url();
    SubResource *subResource = d->mSubResources.value( collectionUrl, 0 );
    if ( subResource == 0 ) {
      subResource = new SubResource( collection, d->mConfig );
      d->mSubResources.insert( collectionUrl, subResource );
      d->mSubResourceIds.insert( collectionUrl );
      kDebug(5700) << "Adding subResource" << subResource->mLabel
                   << "for collection" << collection.url();

      emit signalSubresourceAdded( this, QLatin1String( "contact" ), collectionUrl );
    }

    // TODO should check whether the model signal handling has already fetched the
    // collection's items
    bool changed = false;
    if ( !d->reloadSubResource( subResource, changed ) )
      result = false;
  }

  if ( result )
    emit loadingFinished( this );

  return result;
}

bool ResourceAkonadi::asyncLoad()
{
  // FIXME: copied from synchronous load
  // we need to get to rid of the model since this kind of usage is out of its
  // scope. Should be replaced with a collection and item aggreation class
  // similar to what the model does but with more control over how it does it.

  // save the list of collections we potentially already have
  Collection::List collections;
  SubResourceMap::const_iterator it    = d->mSubResources.constBegin();
  SubResourceMap::const_iterator endIt = d->mSubResources.constEnd();
  for ( ; it != endIt; ++it ) {
    collections << it.value()->mCollection;
  }

  clear();

  // if we do not have any collection yet, fetch them explicitly
  if ( collections.isEmpty() ) {
    d->mThreadJobContext.clear();
    QFuture<bool> threadResult =
      QtConcurrent::run( &(d->mThreadJobContext), &ThreadJobContext::fetchCollections );
    if ( !threadResult.result() ) {
      emit loadingError( this, d->mThreadJobContext.mJobError );
      return false;
    }

    foreach ( const Collection &collection, d->mThreadJobContext.mCollections ) {
      if ( !collection.contentMimeTypes().contains( QLatin1String( "text/directory" ) )
            && !collection.contentMimeTypes().contains( ContactGroup::mimeType() ) )
        continue;

      collections << collection;
    }
  }

  foreach ( const Collection &collection, collections ) {
    d->mAsyncLoadCollections.insert( collection.url().url() );
  }

  delete d->mCollectionFilterModel;
  delete d->mCollectionModel;

  d->mCollectionModel = new CollectionModel( this );

  d->mCollectionFilterModel = new CollectionFilterProxyModel( this );
  d->mCollectionFilterModel->addMimeTypeFilter( QLatin1String( "text/directory" ) );
  d->mCollectionFilterModel->addMimeTypeFilter( ContactGroup::mimeType() );

  connect( d->mCollectionFilterModel, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
           this, SLOT( collectionRowsInserted( const QModelIndex&, int, int ) ) );
  connect( d->mCollectionFilterModel, SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ),
           this, SLOT( collectionRowsRemoved( const QModelIndex&, int, int ) ) );
  connect( d->mCollectionFilterModel, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( collectionDataChanged( const QModelIndex&, const QModelIndex& ) ) );

  d->mCollectionFilterModel->setSourceModel( d->mCollectionModel );

  if ( d->mAsyncLoadCollections.isEmpty() ) {
    kDebug(5700) << "No collections present at asyncLoad start, emitting loadingFinished";
    emit loadingFinished( this );
  }

  return true;
}

bool ResourceAkonadi::save( Ticket *ticket )
{
  Q_UNUSED( ticket );
  kDebug(5700);

  if ( !d->prepareSaving() )
    return false;

  d->mThreadJobContext.clear();

  d->createItemListsForSave( d->mThreadJobContext.mAddedItems,
                             d->mThreadJobContext.mModifiedItems,
                             d->mThreadJobContext.mDeletedItems );

  QFuture<bool> threadResult =
    QtConcurrent::run( &(d->mThreadJobContext), &ThreadJobContext::saveChanges );
  if ( !threadResult.result() ) {
    // TODO error handling
    kError(5700) << "Save Sequence failed:" << d->mThreadJobContext.mJobError;
    return false;
  }

  d->mChanges.clear();

  return true;
}

bool ResourceAkonadi::asyncSave( Ticket *ticket )
{
  Q_UNUSED( ticket );
  kDebug(5700);

  if ( !d->prepareSaving() )
    return false;

  KJob *job = d->createSaveSequence();
  if ( job == 0 )
    return false;

  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( saveResult( KJob* ) ) );

  job->start();

  return true;
}

void ResourceAkonadi::insertAddressee( const Addressee &addr )
{
  const QString uid = addr.uid();

  // if this is a new addressee or there is no Akonadi side item yet
  // mark it as add, otherwise it is a change
  if ( mAddrMap.constFind( uid ) == mAddrMap.constEnd() ||
       d->mIdMapping.find( uid ) == d->mIdMapping.constEnd() )
    d->mChanges[ uid ] = Private::Added;
  else
    d->mChanges[ uid ] = Private::Changed;

  ResourceABC::insertAddressee( addr );
}

void ResourceAkonadi::removeAddressee( const Addressee &addr )
{
  kDebug(5700);
  const QString uid = addr.uid();
  Addressee::Map::const_iterator findIt = mAddrMap.constFind( uid );
  if ( findIt == mAddrMap.constEnd() )
    return;

  // if the item exists on Akonadi side, mark it for removal, otherwise
  // this is a local change only
  if ( d->mIdMapping.find( uid ) != d->mIdMapping.constEnd() )
    d->mChanges[ uid ] = Private::Removed;
  else
    d->mChanges.remove( uid );

  d->mUidToResourceMap.remove( uid );

  ResourceABC::removeAddressee( addr );
}

void ResourceAkonadi::insertDistributionList( DistributionList *list )
{
  kDebug(5700) << "identifier=" << list->identifier()
               << ", name=" << list->name();

  const QString uid = list->identifier();

  // if this is a new distlist or there is no Akonadi side item yet
  // mark it as add, otherwise it is a change
  if ( mDistListMap.find( uid ) == mDistListMap.end() ||
       d->mIdMapping.find( uid ) == d->mIdMapping.constEnd() )
    d->mChanges[ uid ] = Private::Added;
  else
    d->mChanges[ uid ] = Private::Changed;

  ResourceABC::insertDistributionList( list );
}

void ResourceAkonadi::removeDistributionList( DistributionList *list )
{
  kDebug(5700) << "identifier=" << list->identifier()
               << ", name=" << list->name();

  const QString uid = list->identifier();
  DistributionListMap::const_iterator findIt = mDistListMap.constFind( uid );
  if ( findIt == mDistListMap.constEnd() )
    return;

  // if the item exists on Akonadi side, mark it for removal, otherwise
  // this is a local change only
  if ( d->mIdMapping.find( uid ) != d->mIdMapping.constEnd() )
    d->mChanges[ uid ] = Private::Removed;
  else
    d->mChanges.remove( uid );

  d->mUidToResourceMap.remove( uid );

  ResourceABC::removeDistributionList( list );
}

bool ResourceAkonadi::subresourceActive( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  SubResource *resource = d->mSubResources.value( subResource, 0 );
  if ( resource != 0 )
    return resource->isActive();

  return false;
}

bool ResourceAkonadi::subresourceWritable( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  SubResource *resource = d->mSubResources.value( subResource, 0 );
  if ( resource != 0 )
    return resource->isWritable();

  return false;
}

QString ResourceAkonadi::subresourceLabel( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  SubResource *resource = d->mSubResources.value( subResource, 0 );
  if ( resource != 0 )
    return resource->mLabel;

  return false;
}

int ResourceAkonadi::subresourceCompletionWeight( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  SubResource *resource = d->mSubResources.value( subResource, 0 );
  if ( resource != 0 )
    return resource->completionWeight();

  return false;
}

QStringList ResourceAkonadi::subresources() const
{
  kDebug(5700) << d->mSubResourceIds;
  return d->mSubResourceIds.toList();
}

QMap<QString, QString> ResourceAkonadi::uidToResourceMap() const
{
  return d->mUidToResourceMap;
}

void ResourceAkonadi::setStoreCollection( const Akonadi::Collection& collection )
{
  d->mStoreCollection = collection;
}

Akonadi::Collection ResourceAkonadi::storeCollection() const
{
  if ( d->mStoreCollection.isValid() )
    return d->mStoreCollection;
  else return d->findDefaultCollection();
}

void ResourceAkonadi::setSubresourceActive( const QString &subResource, bool active )
{
  kDebug(5700) << "subResource" << subResource << ", active" << active;

  bool changed = false;

  SubResource *resource = d->mSubResources.value( subResource, 0 );
  if ( resource != 0 ) {
    resource->setActive( active );

    if ( !active ) {
      UidResourceMap::iterator it = d->mUidToResourceMap.begin();
      while ( it != d->mUidToResourceMap.end() ) {
        if ( it.value() == subResource ) {
          changed = true;

          mAddrMap.remove( it.key() );
          mDistListMap.remove( it.key() );
          it = d->mUidToResourceMap.erase( it );
        }
        else
          ++it;
      }
    } else {
      d->reloadSubResource( resource, changed );
    }
  }

  if ( changed )
    addressBook()->emitAddressBookChanged();
}

void ResourceAkonadi::setSubresourceCompletionWeight( const QString &subResource, int weight )
{
  kDebug(5700) << "subResource" << subResource << ", weight" << weight;

  SubResource *resource = d->mSubResources.value( subResource, 0 );
  if ( resource != 0 )
    resource->setCompletionWeight( weight );
}

void ResourceAkonadi::loadResult( KJob *job )
{
  if ( job->error() != 0 ) {
    emit loadingError( this, job->errorString() );
    return;
  }

  ItemFetchJob *fetchJob = dynamic_cast<ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  Item::List items = fetchJob->items();

  kDebug(5700) << "Item fetch produced" << items.count() << "items";

  foreach ( const Item& item, items ) {
    if ( item.hasPayload<Addressee>() ) {
      Addressee addressee = item.payload<Addressee>();
      addressee.setResource( this );

      const Item::Id id = item.id();
      d->mIdMapping.insert( addressee.uid(), id );

      mAddrMap.insert( addressee.uid(), addressee );
    } else if ( item.hasPayload<ContactGroup>() ) {
      ContactGroup contactGroup = item.payload<ContactGroup>();

      const Item::Id id = item.id();
      d->mIdMapping.insert( contactGroup.id(), id );

      mDistListMap.insert( contactGroup.id(), d->distListFromContactGroup( contactGroup ) );
    }

    // always update the item
    d->mItems.insert( item.id(), item );
  }

  emit loadingFinished( this );
}

void ResourceAkonadi::saveResult( KJob *job )
{
  if ( job->error() != 0 ) {
    emit savingError( this, job->errorString() );
  } else {
    d->mChanges.clear();

    emit savingFinished( this );
  }
}

void ResourceAkonadi::Private::subResourceLoadResult( KJob *job )
{
  if ( job->error() != 0 ) {
    emit mParent->loadingError( mParent, job->errorString() );
    return;
  }

  ItemFetchJob *fetchJob = dynamic_cast<ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  const QString collectionUrl = mJobToResourceMap.take( fetchJob );
  Q_ASSERT( !collectionUrl.isEmpty() );

  Item::List items = fetchJob->items();

  kDebug(5700) << "Item fetch for sub resource " << collectionUrl
               << "produced" << items.count() << "items";

  foreach ( const Item& item, items ) {
    if ( item.hasPayload<Addressee>() ) {
      Addressee addressee = item.payload<Addressee>();
      addressee.setResource( mParent );

      const Item::Id id = item.id();
      mIdMapping.insert( addressee.uid(), id );

      mParent->mAddrMap.insert( addressee.uid(), addressee );
      mUidToResourceMap.insert( addressee.uid(), collectionUrl );
      mItemIdToResourceMap.insert( id, collectionUrl );
      mChanges.remove( addressee.uid() );
    } else if ( item.hasPayload<ContactGroup>() ) {
      ContactGroup contactGroup = item.payload<ContactGroup>();

      const Item::Id id = item.id();
      mIdMapping.insert( contactGroup.id(), id );

      mParent->mDistListMap.insert( contactGroup.id(), distListFromContactGroup( contactGroup ) );
      mUidToResourceMap.insert( contactGroup.id(), collectionUrl );
      mItemIdToResourceMap.insert( id, collectionUrl );
      mChanges.remove( contactGroup.id() );
    }

    // always update the item
    mItems.insert( item.id(), item );
  }

  mParent->addressBook()->emitAddressBookChanged();
  emit mParent->signalSubresourceAdded( mParent, QLatin1String( "contact" ), collectionUrl );

  if ( !mAsyncLoadCollections.isEmpty() ) {
    mAsyncLoadCollections.remove( collectionUrl );

    if ( mAsyncLoadCollections.isEmpty() ) {
      kDebug(5700) << "All collections present at asyncLoad start have been loaded, emitting loadingFinished";
      emit mParent->loadingFinished( mParent );
    }
  }
}

void ResourceAkonadi::Private::itemAdded( const Akonadi::Item &item,
                                          const Akonadi::Collection &collection )
{
  kDebug(5700);
  const QString collectionUrl = collection.url().url();
  SubResource *subResource = mSubResources.value( collectionUrl, 0 );
  if ( subResource == 0 || !subResource->isActive() )
    return;

  if ( item.hasPayload<Addressee>() ) {
    Addressee addressee = item.payload<Addressee>();
    addressee.setResource( mParent );

    kDebug(5700) << "Addressee" << addressee.uid() << "("
                 << addressee.formattedName() << ")";

    const Item::Id id = item.id();
    mIdMapping.insert( addressee.uid(), id );
    mUidToResourceMap.insert( addressee.uid(), collectionUrl );
    mItemIdToResourceMap.insert( id, collectionUrl );

    mItems.insert( id, item );

    // might be the result of our own saving
    if ( mParent->mAddrMap.find( addressee.uid() ) == mParent->mAddrMap.end() ) {
      mChanges.remove( addressee.uid() );
      mParent->mAddrMap.insert( addressee.uid(), addressee );

      mParent->addressBook()->emitAddressBookChanged();
    }
  } else if ( item.hasPayload<ContactGroup>() ) {
    ContactGroup contactGroup = item.payload<ContactGroup>();

    kDebug(5700) << "ContactGroup" << contactGroup.id() << "("
                 << contactGroup.name() << ")";

    const Item::Id id = item.id();
    mIdMapping.insert( contactGroup.id(), id );
    mUidToResourceMap.insert( contactGroup.id(), collectionUrl );
    mItemIdToResourceMap.insert( id, collectionUrl );

    mItems.insert( id, item );

    // might be the result of our own saving
    if ( mParent->mDistListMap.find( contactGroup.id() ) == mParent->mDistListMap.end() ) {
      mParent->mDistListMap.insert( contactGroup.id(), distListFromContactGroup( contactGroup ) );
      mChanges.remove( contactGroup.id() );

      mParent->addressBook()->emitAddressBookChanged();
    }
  } else {
    kError(5700) << "item has neither addressee nor contact group payload";
  }
}

void ResourceAkonadi::Private::itemChanged( const Akonadi::Item &item,
                                            const QSet<QByteArray> &partIdentifiers )
{
  kDebug(5700) << partIdentifiers;

  ItemMap::iterator itemIt = mItems.find( item.id() );
  if ( itemIt == mItems.end() || !( itemIt.value() == item ) ) {
    kWarning(5700) << "No matching local item for item: id="
                   << item.id() << ", remoteId="
                   << item.remoteId();
    return;
  }

  itemIt.value() = item;

  const QString collectionUrl = mItemIdToResourceMap.value( item.id() );
  SubResource *subResource = mSubResources.value( collectionUrl, 0 );
  if ( subResource == 0 || !subResource->isActive() )
    return;

  if ( !partIdentifiers.contains( Akonadi::Item::FullPayload ) ) {
    kDebug(5700) << "No update to the item body";
    // FIXME find out why payload updates do not contain PartBody?
    //return;
  }

  if ( item.hasPayload<Addressee>() ) {
    Addressee addressee = item.payload<Addressee>();
    addressee.setResource( mParent );

    kDebug(5700) << "Addressee" << addressee.uid() << "("
                 << addressee.formattedName() << ")";

    Addressee::Map::iterator addrIt = mParent->mAddrMap.find( addressee.uid() );
    if ( addrIt == mParent->mAddrMap.end() ) {
      kWarning(5700) << "Addressee  " << addressee.uid() << "("
                     << addressee.formattedName()
                     << ") changed but no longer in local list";
      return;
    }

    mChanges.remove( addressee.uid() );

    if ( addrIt.value() == addressee ) {
      kDebug(5700) << "Local addressee object already up-to-date";
      return;
    }

    addrIt.value() = addressee;

    mParent->addressBook()->emitAddressBookChanged();
  } else if ( item.hasPayload<ContactGroup>() ) {
    ContactGroup contactGroup = item.payload<ContactGroup>();

    kDebug(5700) << "ContactGroup" << contactGroup.id() << "("
                 << contactGroup.name() << ")";

    DistributionListMap::iterator distListIt = mParent->mDistListMap.find( contactGroup.id() );
    if ( distListIt == mParent->mDistListMap.end() ) {
      kWarning(5700) << "DistributionList  " << contactGroup.id() << "("
                     << contactGroup.name()
                     << ") changed but no longer in local list";
      return;
    }

    // TODO: check if we can compare DistributionList and ContactGroup for
    // and equivalent check
//     if ( addrIt.value() == addressee ) {
//       kDebug(5700) << "Local addressee object already up-to-date";
//       return;
//     }

    DistributionList *list = distListIt.value();
    delete list;
    list = distListFromContactGroup( contactGroup );
    distListIt.value() = list;

    // now remove the Change marking caused by implicit remove/add above
    mChanges.remove( contactGroup.id() );

    mParent->addressBook()->emitAddressBookChanged();
  } else {
    kError(5700) << "item has neither addressee nor contact group payload";
  }
}

void ResourceAkonadi::Private::itemRemoved( const Akonadi::Item &item )
{
  kDebug(5700);

  const Item::Id id = item.id();

  const QString collectionUrl = mItemIdToResourceMap.value( id );
  SubResource *subResource = mSubResources.value( collectionUrl, 0 );
  if ( subResource == 0 || !subResource->isActive() )
    return;

  ItemMap::iterator itemIt = mItems.find( id );
  if ( itemIt == mItems.end() )
    return;

  const Item oldItem = itemIt.value();
  if ( oldItem != item )
    return;

  QString uid;
  if ( oldItem.hasPayload<Addressee>() ) {
    uid = oldItem.payload<Addressee>().uid();
  } else if ( oldItem.hasPayload<ContactGroup>() ) {
    uid = oldItem.payload<ContactGroup>().id();
  } else {
    // since we always fetch the payload this should not happen
    // but we really do not want stale entries
    kWarning(5700) << "No Addressee or ContactGroup in local item: id=" << id
                   << ", remoteId=" << oldItem.remoteId();

    IdHash::const_iterator idIt    = mIdMapping.constBegin();
    IdHash::const_iterator idEndIt = mIdMapping.constEnd();
    for ( ; idIt != idEndIt; ++idIt ) {
      if ( idIt.value() == id ) {
        uid = idIt.key();
        break;
      }
    }

    // if there is no mapping we already removed it locally
    if ( uid.isEmpty() )
      return;
  }

  mItems.erase( itemIt );
  mIdMapping.remove( uid );
  mUidToResourceMap.remove( uid );
  mItemIdToResourceMap.remove( id );

  bool changed = false;

  Addressee::Map::iterator addrIt = mParent->mAddrMap.find( uid );
  // if it does not exist as an addressee we already removed it locally
  if ( addrIt != mParent->mAddrMap.end() ) {
    kDebug(5700) << "Addressee" << uid << "("
                 << addrIt.value().formattedName() << ")";

    mParent->mAddrMap.erase( addrIt );

    changed = true;
  }

  DistributionListMap::iterator distListIt = mParent->mDistListMap.find( uid );
  // if it does not exist as a distribution list we already removed it locally
  if ( distListIt != mParent->mDistListMap.end() ) {
    DistributionList *list = distListIt.value();
    kDebug(5700) << "DistributionList" << uid << "("
                 << list->name() << ")";

    mParent->mDistListMap.erase( distListIt );
    delete list;

    changed = true;
  }

  mChanges.remove( uid );

  if ( changed )
    mParent->addressBook()->emitAddressBookChanged();
}

void ResourceAkonadi::Private::collectionRowsInserted( const QModelIndex &parent, int start, int end )
{
  kDebug(5700) << "start" << start << ", end" << end;

  addCollectionsRecursively( parent, start, end );
}

void ResourceAkonadi::Private::collectionRowsRemoved( const QModelIndex &parent, int start, int end )
{
  kDebug(5700) << "start" << start << ", end" << end;

  if ( removeCollectionsRecursively( parent, start, end ) )
    mParent->addressBook()->emitAddressBookChanged();
}

void ResourceAkonadi::Private::collectionDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight )
{
  const int start = topLeft.row();
  const int end   = bottomRight.row();
  kDebug(5700) << "start=" << start << ", end=" << end;

  bool changed = false;
  for ( int row = start; row <= end; ++row ) {
    QModelIndex index = mCollectionFilterModel->index( row, 0, topLeft.parent() );
    if ( index.isValid() ) {
      QVariant data = mCollectionFilterModel->data( index, CollectionModel::CollectionRole );
      if ( data.isValid() ) {
        Collection collection = data.value<Collection>();
        if ( collection.isValid() ) {
          const QString collectionUrl = collection.url().url();
          SubResource* subResource = mSubResources.value( collectionUrl, 0 );
          if ( subResource != 0 ) {
            QString newName = collection.name();
            if ( collection.hasAttribute<EntityDisplayAttribute>() &&
                 !collection.attribute<EntityDisplayAttribute>()->displayName().isEmpty() )
              newName = collection.attribute<EntityDisplayAttribute>()->displayName();
            if ( subResource->mLabel != newName ) {
              kDebug(5700) << "Renaming subResource" << collectionUrl
                           << "from" << subResource->mLabel
                           << "to"   << newName;
              subResource->mLabel = newName;
              changed = true;
              emit mParent->signalSubresourceChanged( mParent, QLatin1String( "contact" ), collectionUrl );
            }
          }
        }
      }
    }
  }

  if ( changed )
    mParent->addressBook()->emitAddressBookChanged();
}

void ResourceAkonadi::Private::addCollectionsRecursively( const QModelIndex &parent, int start, int end )
{
  kDebug(5700) << "start=" << start << ", end=" << end;
  for ( int row = start; row <= end; ++row ) {
    QModelIndex index = mCollectionFilterModel->index( row, 0, parent );
    if ( index.isValid() ) {
      QVariant data = mCollectionFilterModel->data( index, CollectionModel::CollectionRole );
      if ( data.isValid() ) {
        // TODO: ignore virtual collections, since they break Uid to Resource mapping
        // and the application can't handle them anyway
        Collection collection = data.value<Collection>();
        if ( collection.isValid() ) {
          const QString collectionUrl = collection.url().url();

          SubResource *subResource = mSubResources.value( collectionUrl, 0 );
          if ( subResource == 0 ) {
            subResource = new SubResource( collection, mConfig );
            mSubResources.insert( collectionUrl, subResource );
            mSubResourceIds.insert( collectionUrl );
            kDebug(5700) << "Adding subResource" << subResource->mLabel
                         << "for collection" << collection.url();

            ItemFetchJob *job = new ItemFetchJob( collection );
            job->fetchScope().fetchFullPayload();

            connect( job, SIGNAL( result( KJob* ) ),
                     mParent, SLOT( subResourceLoadResult( KJob* ) ) );

            mJobToResourceMap.insert( job, collectionUrl );

            // check if this item has children
            const int rowCount = mCollectionFilterModel->rowCount( index );
            if ( rowCount > 0 )
              addCollectionsRecursively( index, 0, rowCount - 1 );
          }
        }
      }
    }
  }
}

bool ResourceAkonadi::Private::removeCollectionsRecursively( const QModelIndex &parent, int start, int end )
{
  kDebug(5700) << "start=" << start << ", end=" << end;

  bool changed = false;
  for ( int row = start; row <= end; ++row ) {
    QModelIndex index = mCollectionFilterModel->index( row, 0, parent );
    if ( index.isValid() ) {
      QVariant data = mCollectionFilterModel->data( index, CollectionModel::CollectionRole );
      if ( data.isValid() ) {
        Collection collection = data.value<Collection>();
        if ( collection.isValid() ) {
          const QString collectionUrl = collection.url().url();
          mSubResourceIds.remove( collectionUrl );
          SubResource* subResource = mSubResources.take( collectionUrl );
          if ( subResource != 0 ) {
            UidResourceMap::iterator uidIt = mUidToResourceMap.begin();
            while ( uidIt != mUidToResourceMap.end() ) {
              if ( uidIt.value() == collectionUrl ) {
                changed = true;

                // save current uid, then change iterator
                const QString uid = uidIt.key();
                uidIt = mUidToResourceMap.erase( uidIt );

                mParent->mAddrMap.remove( uid );
                DistributionList *list = mParent->mDistListMap.value( uid, 0 );
                delete list;
                mChanges.remove( uid );
              }
              else
                ++uidIt;
            }

            ItemIdResourceMap::iterator idIt = mItemIdToResourceMap.begin();
            while ( idIt != mItemIdToResourceMap.end() ) {
              if ( idIt.value() == collectionUrl ) {
                idIt = mItemIdToResourceMap.erase( idIt );
              }
              else
                ++idIt;
            }

            emit mParent->signalSubresourceRemoved( mParent, QLatin1String( "contact" ), collectionUrl );

            delete subResource;

            const int rowCount = mCollectionFilterModel->rowCount( index );
            if ( rowCount > 0 )
              removeCollectionsRecursively( index, 0, rowCount - 1 );
          }

          if ( !mAsyncLoadCollections.isEmpty() ) {
            mAsyncLoadCollections.remove( collectionUrl );
            if ( mAsyncLoadCollections.isEmpty() ) {
              kDebug(5700) << "Last collection present at asyncLoad start has been removed, emitting loadingFinished";
              emit mParent->loadingFinished( mParent );
            }
          }
        }
      }
    }
  }

  return changed;
}

Collection ResourceAkonadi::Private::findDefaultCollection() const
{
  if ( mConfig.isValid() ) {
    const QString keyName( "DefaultAkonadiResourceIdentifier" );
    if ( mConfig.hasKey( keyName ) ) {
      const QString akonadiAgentIdentifier = mConfig.readEntry( keyName );
      foreach( const SubResource *subResource, mSubResources ) {
        if ( subResource->collection().resource() == akonadiAgentIdentifier ) {
          return subResource->collection();
        }
      }
    }
  }
  return Collection();
}

bool ResourceAkonadi::Private::prepareSaving()
{
  // if an addressee is not yet mapped to one of the sub resources we put it into
  // our store collection.
  // if the store collection is no or nor longer valid, ask for a new one
  Addressee::Map::const_iterator addrIt    = mParent->mAddrMap.constBegin();
  Addressee::Map::const_iterator addrEndIt = mParent->mAddrMap.constEnd();
  while ( addrIt != addrEndIt ) {
    UidResourceMap::const_iterator findIt = mUidToResourceMap.constFind( addrIt.key() );
    if ( findIt == mUidToResourceMap.constEnd() ) {
      if ( !mStoreCollection.isValid() ||
           mSubResources.value( mStoreCollection.url().url(), 0 ) == 0 ) {

        // if there is only one subresource take it instead of asking
        // since this is the most likely choice of the user anyway
        if ( mSubResourceIds.count() == 1 ) {
          mStoreCollection = Collection::fromUrl( *mSubResourceIds.constBegin() );
        } else {
          Collection defaultCollection = mParent->storeCollection();
          if ( defaultCollection.isValid() ) {
            mParent->setStoreCollection( defaultCollection );
          }
          else {
            ResourceAkonadiConfigDialog dialog( mParent );
            if ( dialog.exec() != QDialog::Accepted )
              return false;
          }
        }

        // if accepted, use the same iterator position again to re-check
      } else {
        mUidToResourceMap.insert( addrIt.key(), mStoreCollection.url().url() );
        ++addrIt;
      }
    } else
      ++addrIt;
  }

  // TODO: should probably ask for a store collection as well if a distribution list
  // is new
  // for now check if we have other dist lists and where they are stored, then
  // check addressee store collection
  QString distListStoreCollection;
  DistributionListMap::const_iterator distListIt    = mParent->mDistListMap.constBegin();
  DistributionListMap::const_iterator distListEndIt = mParent->mDistListMap.constEnd();
  while ( distListIt != distListEndIt ) {
    UidResourceMap::const_iterator findIt = mUidToResourceMap.constFind( distListIt.key() );
    if ( findIt == mUidToResourceMap.constEnd() ) {
      if ( distListStoreCollection.isEmpty() ) {
        DistributionListMap::const_iterator searchIt = distListIt;
        for ( ; searchIt != distListEndIt; ++searchIt ) {
          UidResourceMap::const_iterator findIt2 = mUidToResourceMap.constFind( searchIt.key() );
          if ( findIt2 != mUidToResourceMap.constEnd() ) {
            distListStoreCollection = findIt2.value();
            break;
          }
        }
      }

      if ( distListStoreCollection.isEmpty() ) {
        DistributionList *list = distListIt.value();
        Q_ASSERT( list != 0 );

        DistributionList::Entry::List entries = list->entries();
        foreach ( const DistributionList::Entry &entry, entries ) {
          Addressee addressee = entry.addressee();
          if ( !addressee.isEmpty() ) {
            UidResourceMap::const_iterator findIt2 = mUidToResourceMap.constFind( addressee.uid() );
            if ( findIt2 != mUidToResourceMap.constEnd() ) {
              // check if the collection can store contact group items as well
              SubResource *subResource = mSubResources.value( findIt2.value(), 0 );
              if ( subResource != 0 ) {
                if ( subResource->mCollection.contentMimeTypes().contains( ContactGroup::mimeType() ) ) {
                  distListStoreCollection = findIt2.value();
                  break;
                }
              }
            }
          }
        }
      }

      if ( !distListStoreCollection.isEmpty() ) {
        mUidToResourceMap.insert( distListIt.key(), distListStoreCollection );
        ++distListIt;
      } else {
        kError() << "Could not determine a collection for saving distlists";
        return false;
      }
    } else {
      distListStoreCollection = findIt.value();
      ++distListIt;
    }
  }

  return true;
}

static KJob *createSaveSequence( const UidToCollectionMapper &mapper,
    const Item::List &addedItems, const Item::List &modifiedItems, const Item::List &deletedItems )
{
  TransactionSequence *sequence = new TransactionSequence();

  foreach ( const Item &item, addedItems ) {
    QString uid;
    if ( item.hasPayload<KABC::Addressee>() ) {
      const KABC::Addressee addressee = item.payload<KABC::Addressee>();
      kDebug(5700) << "CreateJob for addressee" << addressee.uid()
                   << addressee.formattedName();
      uid = addressee.uid();
    } else if ( item.hasPayload<KABC::ContactGroup>() ) {
      const KABC::ContactGroup contactGroup = item.payload<KABC::ContactGroup>();
      kDebug(5700) << "CreateJob for contact group" << contactGroup.id()
                   << contactGroup.name();
      uid = contactGroup.id();
    } else {
      kWarning(5700) << "New item id=" << item.id() << "neither addressee nor contactgroup";
      continue;
    }

    (void) new ItemCreateJob( item, mapper.collectionForUid( uid ), sequence );
  }

  foreach ( const Item &item, modifiedItems ) {
    if ( item.hasPayload<KABC::Addressee>() ) {
      const KABC::Addressee addressee = item.payload<KABC::Addressee>();
      kDebug(5700) << "ModifyJob for addressee" << addressee.uid()
                   << addressee.formattedName();
    } else if ( item.hasPayload<KABC::ContactGroup>() ) {
      const KABC::ContactGroup contactGroup = item.payload<KABC::ContactGroup>();
      kDebug(5700) << "ModifyJob for contact group" << contactGroup.id()
                   << contactGroup.name();
    } else {
      kWarning(5700) << "Modified item id=" << item.id() << "neither addressee nor contactgroup";
      continue;
    }

    (void) new ItemModifyJob( item, sequence );
  }

  foreach ( const Item &item, deletedItems ) {
    if ( item.hasPayload<KABC::Addressee>() ) {
      const KABC::Addressee addressee = item.payload<KABC::Addressee>();
      kDebug(5700) << "DeleteJob for addressee" << addressee.uid()
                   << addressee.formattedName();
    } else if ( item.hasPayload<KABC::ContactGroup>() ) {
      const KABC::ContactGroup contactGroup = item.payload<KABC::ContactGroup>();
      kDebug(5700) << "DeleteJob for contact group" << contactGroup.id()
                   << contactGroup.name();
    } else {
      kWarning(5700) << "Deleted item id=" << item.id() << "neither addressee nor contactgroup";
      continue;
    }

    (void) new ItemDeleteJob( item, sequence );
  }

  return sequence;
}

KJob *ResourceAkonadi::Private::createSaveSequence() const
{
  Item::List addedItems;
  Item::List modifiedItems;
  Item::List deletedItems;

  createItemListsForSave( addedItems, modifiedItems, deletedItems );

  return ::createSaveSequence( *this, addedItems, modifiedItems, deletedItems );
}

void ResourceAkonadi::Private::createItemListsForSave( Item::List &added, Item::List &modified, Item::List &deleted ) const
{
  kDebug(5700) << mChanges.count() << "changes";

  ChangeMap::const_iterator changeIt    = mChanges.constBegin();
  ChangeMap::const_iterator changeEndIt = mChanges.constEnd();
  for ( ; changeIt != changeEndIt; ++changeIt ) {
    const QString uid = changeIt.key();

    IdHash::const_iterator idIt = mIdMapping.find( uid );

    ItemMap::const_iterator itemIt;

    Item item;
    SubResource *subResource = 0;
    KABC::Addressee addressee;

    switch ( changeIt.value() ) {
      case Added:
        subResource = mSubResources.value( mUidToResourceMap.value( uid ), 0 );
        Q_ASSERT( subResource != 0 );

        addressee = mParent->mAddrMap.value( uid );
        if ( !addressee.isEmpty() ) {
          item.setMimeType( QLatin1String( "text/directory" ) );
          item.setPayload<KABC::Addressee>( addressee );

          added << item;
        } else {
          DistributionList *list = mParent->mDistListMap.value( uid );
          if ( list != 0 ) {
            ContactGroup contactGroup = contactGroupFromDistList( list );

            item.setMimeType( ContactGroup::mimeType() );
            item.setPayload<ContactGroup>( contactGroup );

            added << item;
          }
        }
        break;

      case Changed:
        Q_ASSERT( idIt != mIdMapping.constEnd() );
        itemIt = mItems.find( idIt.value() );
        Q_ASSERT( itemIt != mItems.constEnd() );

        item = itemIt.value();

        if ( item.hasPayload<KABC::Addressee>() ) {
          addressee = mParent->mAddrMap.value( uid );

          item.setPayload<KABC::Addressee>( addressee );

          modified << item;
        } else if ( item.hasPayload<KABC::ContactGroup>() ) {
          DistributionList *list = mParent->mDistListMap.value( uid );
          if ( list != 0 ) {
            ContactGroup contactGroup = contactGroupFromDistList( list );

            item.setPayload<KABC::ContactGroup>( contactGroup );

            modified << item;
          }
        }
        break;

      case Removed:
        Q_ASSERT( idIt != mIdMapping.constEnd() );
        itemIt = mItems.find( idIt.value() );
        Q_ASSERT( itemIt != mItems.constEnd() );

        item = itemIt.value();
        deleted << item;
        break;
    }
  }
}

bool ResourceAkonadi::Private::reloadSubResource( SubResource *subResource, bool &changed )
{
  changed = false;
  mThreadJobContext.clear();
  QFuture<bool> threadResult =
    QtConcurrent::run( &mThreadJobContext, &ThreadJobContext::fetchItems, subResource->mCollection );
  if ( !threadResult.result() ) {
    emit mParent->loadingError( mParent, mThreadJobContext.mJobError );
    return false;
  }

  Item::List items = mThreadJobContext.mItems;

  const QString collectionUrl = subResource->mCollection.url().url();

  kDebug(5700) << "Reload for sub resource " << collectionUrl
               << "produced" << items.count() << "items";

  foreach ( const Item& item, items ) {
    if ( item.hasPayload<Addressee>() ) {
      changed = true;

      Addressee addressee = item.payload<Addressee>();
      addressee.setResource( mParent );

      const Item::Id id = item.id();
      mIdMapping.insert( addressee.uid(), id );

      mParent->mAddrMap.insert( addressee.uid(), addressee );
      mUidToResourceMap.insert( addressee.uid(), collectionUrl );
      mItemIdToResourceMap.insert( id, collectionUrl );
    } else if ( item.hasPayload<KABC::ContactGroup>() ) {
      changed = true;

      ContactGroup contactGroup = item.payload<ContactGroup>();

      DistributionList *list = distListFromContactGroup( contactGroup );

      const Item::Id id = item.id();
      mIdMapping.insert( contactGroup.id(), id );

      // TODO can there be one in the map already? if yes, do we delete or update it?
      mParent->mDistListMap.insert( contactGroup.id(), list );
      mUidToResourceMap.insert( contactGroup.id(), collectionUrl );
      mItemIdToResourceMap.insert( id, collectionUrl );
    }

    // always update the item
    mItems.insert( item.id(), item );
  }

  return true;
}

DistributionList *ResourceAkonadi::Private::distListFromContactGroup( const ContactGroup &contactGroup )
{
  DistributionList *list = new DistributionList( mParent, contactGroup.id(), contactGroup.name() );

  for ( unsigned int refIndex = 0; refIndex < contactGroup.contactReferenceCount(); ++refIndex ) {
    const ContactGroup::ContactReference &reference = contactGroup.contactReference( refIndex );

    Addressee addressee;
    Addressee::Map::const_iterator it = mParent->mAddrMap.constFind( reference.uid() );
    if ( it == mParent->mAddrMap.constEnd() ) {
      addressee.setUid( reference.uid() );

      // TODO any way to set a good name?
    } else
      addressee = it.value();

    // TODO how to handle ContactGroup::Reference custom fields?

    list->insertEntry( addressee, reference.preferredEmail() );
  }

  for ( unsigned int dataIndex = 0; dataIndex < contactGroup.dataCount(); ++dataIndex ) {
    const ContactGroup::Data &data = contactGroup.data( dataIndex );

    Addressee addressee;
    addressee.setName( data.name() );
    addressee.insertEmail( data.email() );

    // TODO how to handle ContactGroup::Data custom fields?

    list->insertEntry( addressee );
  }

  return list;
}

ContactGroup ResourceAkonadi::Private::contactGroupFromDistList( const KABC::DistributionList* list ) const
{
  ContactGroup contactGroup( list->name() );
  contactGroup.setId( list->identifier() );

  DistributionList::Entry::List entries = list->entries();
  foreach ( const DistributionList::Entry &entry, entries ) {
    Addressee addressee = entry.addressee();
    const QString email = entry.email();
    if ( addressee.isEmpty() ) {
      if ( email.isEmpty() )
        continue;

      ContactGroup::Data data( email, email );
      contactGroup.append( data );
    } else {
      Addressee baseAddressee = mParent->mAddrMap.value( addressee.uid() );
      if ( baseAddressee.isEmpty() ) {
        ContactGroup::Data data( email, email );
        // TODO: transer custom fields?
        contactGroup.append( data );
      } else {
        ContactGroup::ContactReference reference( addressee.uid() );
        reference.setPreferredEmail( email );
        // TODO: transer custom fields?
        contactGroup.append( reference );
      }
    }
  }

  return contactGroup;
}

#include "resourceakonadi.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
