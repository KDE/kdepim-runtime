// -*- c-basic-offset: 2 -*-
/*
    This file is part of libkabc.
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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
#include <akonadi/monitor.h>
#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/transactionsequence.h>

#include <kconfiggroup.h>
#include <kdebug.h>

#include <QHash>

using namespace Akonadi;
using namespace KABC;

typedef QMap<Item::Id, Item> ItemMap;
typedef QHash<QString, Item::Id> IdHash;

class SubResource
{
  public:
    SubResource( const Collection &collection, const KConfigGroup &parentGroup )
        : mCollection( collection ), mLabel( collection.name() ),
          mActive(true), mCompletionWeight(80)
    {
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

  public:
    Collection mCollection;
    QString mLabel;

    bool mActive;
    int  mCompletionWeight;
};

typedef QHash<QString, SubResource*> SubResourceMap;

typedef QMap<QString, QString>  UidResourceMap;
typedef QMap<Item::Id, QString> ItemIdResourceMap;

class ResourceAkonadi::Private
{
  public:
    Private( ResourceAkonadi *parent )
      : mParent( parent ), mMonitor( 0 ), mCollectionModel( 0 ), mCollectionFilterModel( 0 )
    {
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

    bool prepareSaving();
    KJob *createSaveSequence() const;

    bool reloadSubResource( SubResource *subResource, bool &changed );
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

  ResourceABC::clear();
}

void ResourceAkonadi::writeConfig( KConfigGroup &group )
{
  ResourceABC::writeConfig( group );

  group.writeEntry( QLatin1String( "CollectionUrl" ), d->mStoreCollection.url() );

  SubResourceMap::const_iterator it    = d->mSubResources.begin();
  SubResourceMap::const_iterator endIt = d->mSubResources.end();
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

  connect( d->mCollectionFilterModel, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
           this, SLOT( collectionRowsInserted( const QModelIndex&, int, int ) ) );
  connect( d->mCollectionFilterModel, SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ),
           this, SLOT( collectionRowsRemoved( const QModelIndex&, int, int ) ) );
  connect( d->mCollectionFilterModel, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( collectionDataChanged( const QModelIndex&, const QModelIndex& ) ) );

  d->mCollectionFilterModel->setSourceModel( d->mCollectionModel );

  d->mMonitor = new Monitor( this );

  d->mMonitor->setMimeTypeMonitored( QLatin1String( "text/directory" ) );
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
  SubResourceMap::const_iterator it    = d->mSubResources.begin();
  SubResourceMap::const_iterator endIt = d->mSubResources.end();
  for ( ; it != endIt; ++it ) {
    collections << it.value()->mCollection;
  }

  clear();

  // if we do not have any collection yet, fetch them explicitly
  if ( collections.isEmpty() ) {
    CollectionFetchJob *colJob =
      new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
    if ( !colJob->exec() ) {
      emit loadingError( this, colJob->errorString() );
      return false;
    }

    // TODO: should probably add the data to the model right here
    collections = colJob->collections();
  }

  bool result = true;
  foreach ( const Collection &collection, collections ) {
     if ( !collection.contentMimeTypes().contains( QLatin1String( "text/directory" ) ) )
       continue;

    const QString collectionUrl = collection.url().url();
    SubResource *subResource = d->mSubResources[ collectionUrl ];
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
  clear();

  delete d->mCollectionFilterModel;
  delete d->mCollectionModel;

  d->mCollectionModel = new CollectionModel( this );

  d->mCollectionFilterModel = new CollectionFilterProxyModel( this );
  d->mCollectionFilterModel->addMimeTypeFilter( QLatin1String( "text/directory" ) );

  connect( d->mCollectionFilterModel, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
           this, SLOT( collectionRowsInserted( const QModelIndex&, int, int ) ) );
  connect( d->mCollectionFilterModel, SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ),
           this, SLOT( collectionRowsRemoved( const QModelIndex&, int, int ) ) );
  connect( d->mCollectionFilterModel, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( collectionDataChanged( const QModelIndex&, const QModelIndex& ) ) );

  d->mCollectionFilterModel->setSourceModel( d->mCollectionModel );

  return true;
}

bool ResourceAkonadi::save( Ticket *ticket )
{
  Q_UNUSED( ticket );
  kDebug(5700);

  if ( !d->prepareSaving() )
    return false;

  KJob *job = d->createSaveSequence();
  if ( job == 0 )
    return false;

  if ( !job->exec() ) {
    // TODO error handling
    kError(5700) << "Save Sequence failed:" << job->errorString();
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
  if ( mAddrMap.find( uid ) == mAddrMap.end() )
    d->mChanges[ uid ] = Private::Added;
  else
    d->mChanges[ uid ] = Private::Changed;

  ResourceABC::insertAddressee( addr );
}

void ResourceAkonadi::removeAddressee( const Addressee &addr )
{
  kDebug(5700);
  const QString uid = addr.uid();
  Addressee::Map::const_iterator findIt = mAddrMap.find( uid );
  if ( findIt == mAddrMap.end() )
    return;

  d->mChanges[ uid ] = Private::Removed;

  d->mUidToResourceMap.remove( uid );

  ResourceABC::removeAddressee( addr );
}

bool ResourceAkonadi::subresourceActive( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  SubResource *resource = d->mSubResources[ subResource ];
  if ( resource != 0 )
    return resource->isActive();

  return false;
}

bool ResourceAkonadi::subresourceWritable( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  SubResource *resource = d->mSubResources[ subResource ];
  if ( resource != 0 )
    return resource->isWritable();

  return false;
}

QString ResourceAkonadi::subresourceLabel( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  SubResource *resource = d->mSubResources[ subResource ];
  if ( resource != 0 )
    return resource->mLabel;

  return false;
}

int ResourceAkonadi::subresourceCompletionWeight( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  SubResource *resource = d->mSubResources[ subResource ];
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
  return d->mStoreCollection;
}

void ResourceAkonadi::setSubresourceActive( const QString &subResource, bool active )
{
  kDebug(5700) << "subResource" << subResource << ", active" << active;

  bool changed = false;

  SubResource *resource = d->mSubResources[ subResource ];
  if ( resource != 0 ) {
    resource->setActive( active );

    if ( !active ) {
      UidResourceMap::iterator it = d->mUidToResourceMap.begin();
      while ( it != d->mUidToResourceMap.end() ) {
        if ( it.value() == subResource ) {
          changed = true;

          mAddrMap.remove( it.key() );
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

  SubResource *resource = d->mSubResources[ subResource ];
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
    }

    // always update the item
    mItems.insert( item.id(), item );
  }

  mParent->addressBook()->emitAddressBookChanged();
  emit mParent->signalSubresourceAdded( mParent, QLatin1String( "contact" ), collectionUrl );
}

void ResourceAkonadi::Private::itemAdded( const Akonadi::Item &item,
                                          const Akonadi::Collection &collection )
{
  kDebug(5700);
  const QString collectionUrl = collection.url().url();
  SubResource *subResource = mSubResources[ collectionUrl ];
  if ( subResource == 0 || !subResource->isActive() )
    return;

  if ( !item.hasPayload<Addressee>() ) {
    kError(5700) << "Item does not have addressee payload";
    return;
  }

  Addressee addressee = item.payload<Addressee>();

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

  const QString collectionUrl = mItemIdToResourceMap[ item.id() ];
  SubResource *subResource = mSubResources[ collectionUrl ];
  if ( subResource == 0 || !subResource->isActive() )
    return;

  if ( !partIdentifiers.contains( Akonadi::Item::FullPayload ) ) {
    kDebug(5700) << "No update to the item body";
    // FIXME find out why payload updates do not contain PartBody?
    //return;
  }

  if ( !item.hasPayload<Addressee>() ) {
    kError(5700) << "Item does not have addressee payload";
    return;
  }

  Addressee addressee = item.payload<Addressee>();

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
}

void ResourceAkonadi::Private::itemRemoved( const Akonadi::Item &item )
{
  kDebug(5700);

  const Item::Id id = item.id();

  const QString collectionUrl = mItemIdToResourceMap[ id ];
  SubResource *subResource = mSubResources[ collectionUrl ];
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
  } else {
    // since we always fetch the payload this should not happen
    // but we really do not want stale entries
    kWarning(5700) << "No Addressee in local item: id=" << id
                   << ", remoteId=" << oldItem.remoteId();

    IdHash::const_iterator idIt    = mIdMapping.begin();
    IdHash::const_iterator idEndIt = mIdMapping.end();
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
  mChanges.remove( uid );

  Addressee::Map::iterator addrIt = mParent->mAddrMap.find( uid );

  // if it does not exist as an addressee we already removed it locally
  if ( addrIt == mParent->mAddrMap.end() )
    return;

  kDebug(5700) << "Addressee" << uid << "("
               << addrIt.value().formattedName() << ")";

  mParent->mAddrMap.erase( addrIt );

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
          SubResource* subResource = mSubResources[ collectionUrl ];
          if ( subResource != 0 ) {
            if ( subResource->mLabel != collection.name() ) {
              kDebug(5700) << "Renaming subResource" << collectionUrl
                           << "from" << subResource->mLabel
                           << "to"   << collection.name();
              subResource->mLabel = collection.name();
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

          SubResource *subResource = mSubResources[ collectionUrl ];
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

                mParent->mAddrMap.remove( uidIt.key() );
                uidIt = mUidToResourceMap.erase( uidIt );
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
        }
      }
    }
  }

  return changed;
}

bool ResourceAkonadi::Private::prepareSaving()
{
  // if an addressee is not yet mapped to one of the sub resources we put it into
  // our store collection.
  // if the store collection is no or nor longer valid, ask for a new one
  Addressee::Map::const_iterator addrIt    = mParent->mAddrMap.begin();
  Addressee::Map::const_iterator addrEndIt = mParent->mAddrMap.end();
  while ( addrIt != addrEndIt ) {
    UidResourceMap::const_iterator findIt = mUidToResourceMap.find( addrIt.key() );
    if ( findIt == mUidToResourceMap.end() ) {
      if ( !mStoreCollection.isValid() ||
           mSubResources[ mStoreCollection.url().url() ] == 0 ) {

        // if there is only one subresource take it instead of asking
        // since this is the most likely choice of the user anyway
        if ( mSubResourceIds.count() == 1 ) {
          mStoreCollection = Collection::fromUrl( *mSubResourceIds.begin() );
        } else {
          ResourceAkonadiConfigDialog dialog( mParent );
          if ( dialog.exec() != QDialog::Accepted )
            return false;
        }

        // if accepted, use the same iterator position again to re-check
      } else {
        mUidToResourceMap.insert( addrIt.key(), mStoreCollection.url().url() );
        ++addrIt;
      }
    } else
      ++addrIt;
  }

  return true;
}

KJob *ResourceAkonadi::Private::createSaveSequence() const
{
  TransactionSequence *sequence = new TransactionSequence();

  ChangeMap::const_iterator changeIt    = mChanges.begin();
  ChangeMap::const_iterator changeEndIt = mChanges.end();
  for ( ; changeIt != changeEndIt; ++changeIt ) {
    const QString uid = changeIt.key();

    IdHash::const_iterator idIt = mIdMapping.find( uid );

    ItemMap::const_iterator itemIt;

    Item item;
    SubResource *subResource = 0;
    KABC::Addressee addressee;

    switch ( changeIt.value() ) {
      case Added:
        subResource = mSubResources[ mUidToResourceMap[ uid ] ];
        Q_ASSERT( subResource != 0 );

        addressee = mParent->mAddrMap[ uid ];

        item.setMimeType( QLatin1String( "text/directory" ) );
        item.setPayload<KABC::Addressee>( addressee );

        (void) new ItemCreateJob( item, subResource->mCollection, sequence );
        kDebug(5700) << "CreateJob for addressee" << addressee.uid()
                     << addressee.formattedName();
        break;

      case Changed:
        Q_ASSERT( idIt != mIdMapping.end() );
        itemIt = mItems.find( idIt.value() );
        Q_ASSERT( itemIt != mItems.end() );

        addressee = mParent->mAddrMap[ uid ];

        item = itemIt.value();
        item.setPayload<KABC::Addressee>( addressee );

        (void) new ItemModifyJob( item, sequence );
        kDebug(5700) << "ModifyJob for addressee" << addressee.uid()
                     << addressee.formattedName();
        break;

      case Removed:
        Q_ASSERT( idIt != mIdMapping.end() );
        itemIt = mItems.find( idIt.value() );
        Q_ASSERT( itemIt != mItems.end() );

        item = itemIt.value();
        if ( item.hasPayload<KABC::Addressee>() )
          addressee = item.payload<KABC::Addressee>();

        (void) new ItemDeleteJob( item, sequence );
        kDebug(5700) << "DeleteJob for addressee" << uid
                     << addressee.formattedName();
        break;
    }
  }

  return sequence;
}

bool ResourceAkonadi::Private::reloadSubResource( SubResource *subResource, bool &changed )
{
  changed = false;
  ItemFetchJob *job = new ItemFetchJob( subResource->mCollection );
  job->fetchScope().fetchFullPayload();

  if ( !job->exec() ) {
    emit mParent->loadingError( mParent, job->errorString() );
    return false;
  }

  const QString collectionUrl = subResource->mCollection.url().url();

  Item::List items = job->items();

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
    }

    // always update the item
    mItems.insert( item.id(), item );
  }

  return true;
}

#include "resourceakonadi.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
