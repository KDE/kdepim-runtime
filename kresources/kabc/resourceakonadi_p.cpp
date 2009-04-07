/*
    This file is part of libkabc.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "resourceakonadi_p.h"

#include "resourceakonadiconfig.h"
#include "storecollectiondialog.h"

#include <akonadi/item.h>

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kabc/distributionlist.h>

using namespace KABC;

ResourceAkonadi::Private::Private( ResourceAkonadi *parent )
  : SharedResourcePrivate<SubResource>( new IdArbiter(), parent ),
    mParent( parent ), mInternalDataChange( false )
{
}

ResourceAkonadi::Private::Private( const KConfigGroup &config, ResourceAkonadi *parent )
  : SharedResourcePrivate<SubResource>( config, new IdArbiter(), parent ),
    mParent( parent ), mInternalDataChange( false )
{
}

bool ResourceAkonadi::Private::insertAddressee( const KABC::Addressee &addressee )
{
  const QString uid = addressee.uid();

  if ( mParent->mAddrMap.constFind( uid ) != mParent->mAddrMap.constEnd() ) {
    changeLocalItem( uid );
    return true;
  }

  return addLocalItem( uid, Addressee::mimeType() );
}

void ResourceAkonadi::Private::removeAddressee( const KABC::Addressee &addressee )
{
  removeLocalItem( addressee.uid() );
}

bool ResourceAkonadi::Private::insertDistributionList( KABC::DistributionList *list )
{
  if ( mInternalDataChange ) {
    return true;
  }

  const QString uid = list->identifier();
  if ( mParent->mDistListMap.constFind( uid ) != mParent->mDistListMap.constEnd() ) {
    changeLocalItem( uid );
    return true;
  }

  return addLocalItem( uid, ContactGroup::mimeType() );

}

void ResourceAkonadi::Private::removeDistributionList( KABC::DistributionList *list )
{
  if ( mInternalDataChange ) {
    return;
  }

  removeLocalItem( list->identifier() );
}

QMap<QString, QString> ResourceAkonadi::Private::uidToResourceMap() const
{
  return mUidToResourceMap;
}

bool ResourceAkonadi::Private::openResource()
{
  kDebug( 5700 );
  return true;
}

bool ResourceAkonadi::Private::closeResource()
{
  kDebug( 5700 );
  // clear local caches
  mParent->mAddrMap.clear();

  // take a copy of mDistListMap, then clear it and finally qDeleteAll
  // the copy to avoid problems with removeDistributionList() called by
  // ~DistributionList().
  BoolGuard internalChange( mInternalDataChange, true );
  DistributionListMap tempDistListMap( mParent->mDistListMap );
  mParent->mDistListMap.clear();
  qDeleteAll( tempDistListMap );

  return true;
}

Akonadi::Item ResourceAkonadi::Private::createItem( const QString &kresId )
{
  Akonadi::Item item;

  DistributionList *list = mParent->mDistListMap.value( kresId, 0 );
  if ( list != 0 ) {
    item.setMimeType( ContactGroup::mimeType() );
    item.setPayload<ContactGroup>( contactGroupFromDistList( list ) );
  } else {
    item.setMimeType( Addressee::mimeType() );
    item.setPayload<Addressee>( mParent->mAddrMap.value( kresId ) );
  }

  return item;
}

Akonadi::Item ResourceAkonadi::Private::updateItem( const Akonadi::Item &item, const QString &kresId, const QString &originalId )
{
  Akonadi::Item update( item );

  DistributionList *list = mParent->mDistListMap.value( kresId, 0 );
  if ( list != 0 ) {
    ContactGroup contactGroup = contactGroupFromDistList( list );
    contactGroup.setId( originalId );

    update.setPayload<ContactGroup>( contactGroup );
  } else {
    Addressee addressee = mParent->mAddrMap.value( kresId );
    addressee.setUid( originalId );
    update.setPayload<Addressee>( addressee );
  }

  return update;
}

void ResourceAkonadi::Private::subResourceAdded( SubResourceBase *subResource )
{
  kDebug( 5700 ) << "id=" << subResource->subResourceIdentifier();

  SharedResourcePrivate<SubResource>::subResourceAdded( subResource );

  connect( subResource, SIGNAL( addresseeAdded( KABC::Addressee, QString ) ),
           this, SLOT( addresseeAdded( KABC::Addressee, QString ) ) );
  connect( subResource, SIGNAL( addresseeChanged( KABC::Addressee, QString ) ),
           this, SLOT( addresseeChanged( KABC::Addressee, QString ) ) );
  connect( subResource, SIGNAL( addresseeRemoved( QString, QString ) ),
           this, SLOT( addresseeRemoved( QString, QString ) ) );

  connect( subResource, SIGNAL( contactGroupAdded( KABC::ContactGroup, QString ) ),
           this, SLOT( contactGroupAdded( KABC::ContactGroup, QString ) ) );
  connect( subResource, SIGNAL( contactGroupChanged( KABC::ContactGroup, QString ) ),
           this, SLOT( contactGroupChanged( KABC::ContactGroup, QString ) ) );
  connect( subResource, SIGNAL( contactGroupRemoved( QString, QString ) ),
           this, SLOT( contactGroupRemoved( QString, QString ) ) );

  emit mParent->signalSubresourceAdded( mParent, QLatin1String( "contact" ), subResource->subResourceIdentifier() );
}

void ResourceAkonadi::Private::subResourceRemoved( SubResourceBase *subResource )
{
  kDebug( 5700 ) << "id=" << subResource->subResourceIdentifier();

  SharedResourcePrivate<SubResource>::subResourceRemoved( subResource );

  disconnect( subResource, SIGNAL( addresseeAdded( KABC::Addressee, QString ) ),
              this, SLOT( addresseeAdded( KABC::Addressee, QString ) ) );
  disconnect( subResource, SIGNAL( addresseeChanged( KABC::Addressee, QString ) ),
              this, SLOT( addresseeChanged( KABC::Addressee, QString ) ) );
  disconnect( subResource, SIGNAL( addresseeRemoved( QString, QString ) ),
              this, SLOT( addresseeRemoved( QString, QString ) ) );

  disconnect( subResource, SIGNAL( contactGroupAdded( KABC::ContactGroup, QString ) ),
              this, SLOT( contactGroupAdded( KABC::ContactGroup, QString ) ) );
  disconnect( subResource, SIGNAL( contactGroupChanged( KABC::ContactGroup, QString ) ),
              this, SLOT( contactGroupChanged( KABC::ContactGroup, QString ) ) );
  disconnect( subResource, SIGNAL( contactGroupRemoved( QString, QString ) ),
              this, SLOT( contactGroupRemoved( QString, QString ) ) );

  // block scope for BoolGuard
  {
    BoolGuard internalChange( mInternalDataChange, true );

    QMap<QString, QString>::iterator it = mUidToResourceMap.begin();
    while ( it != mUidToResourceMap.end() ) {
      if ( it.value() == subResource->subResourceIdentifier() ) {
        const QString uid = it.key();

        mChanges.remove( uid );
        mIdArbiter->removeArbitratedId( uid );

        mParent->mAddrMap.remove( uid );

        DistributionList *distList = mParent->mDistListMap.value( uid, 0 );
        delete distList;

        it = mUidToResourceMap.erase( it );
      } else {
        ++it;
      }
    }
  }

  emit mParent->signalSubresourceRemoved( mParent, QLatin1String( "contact" ), subResource->subResourceIdentifier() );

  emit mParent->addressBook()->emitAddressBookChanged();
}

void ResourceAkonadi::Private::loadingResult( bool ok, const QString &errorString )
{
  SharedResourcePrivate<SubResource>::loadingResult( ok, errorString );

  if ( ok ) {
    emit mParent->loadingFinished( mParent );
    mModel.startMonitoring();
  } else {
    emit mParent->loadingError( mParent, errorString );
  }
}

void ResourceAkonadi::Private::savingResult( bool ok, const QString &errorString )
{
  SharedResourcePrivate<SubResource>::savingResult( ok, errorString );

  if ( ok ) {
    mChanges.clear();
    emit mParent->savingFinished( mParent );
  } else {
    emit mParent->savingError( mParent, errorString );
  }
}

const SubResourceBase *ResourceAkonadi::Private::storeSubResourceFromUser( const QString &uid, const QString &mimeType )
{
  // TODO Strings should reflect whether this is a question for just one
  // item (ask always) or for all of a certain category (ask once)
  Q_UNUSED( uid );

  Q_ASSERT( mStoreCollectionDialog != 0 );

  if ( mimeType == Addressee::mimeType() ) {
    mStoreCollectionDialog->setLabelText( i18nc( "@label where to store a new address book entry", "Please select a storage folder for this contact:" ) );
  } else if ( mimeType == ContactGroup::mimeType() ) {
    mStoreCollectionDialog->setLabelText( i18nc( "@label where to store a new email distribution list", "Please select a storage folder for this distribution list:" ) );
  } else {
    kError( 5700 ) << "Unexpected MIME type:" << mimeType;
    mStoreCollectionDialog->setLabelText( i18nc( "@label", "Please select a storage folder:" ) );
  }

  mStoreCollectionDialog->setMimeType( mimeType );

  const SubResourceBase *resource = 0;
  while ( resource == 0 ) {
    if ( mStoreCollectionDialog->exec() != QDialog::Accepted ) {
      return 0;
    }

    Akonadi::Collection collection = mStoreCollectionDialog->selectedCollection();
    if ( collection.isValid() ) {
      resource = mModel.subResource( collection.id() );
      if ( resource != 0 ) {
        setStoreCollectionForMimeType( mimeType, collection );
      }
    }
  }

  return resource;
}

void ResourceAkonadi::Private::subResourceChanged( const QString &subResourceIdentifier )
{
  emit mParent->signalSubresourceChanged( mParent, QLatin1String( "contact" ), subResourceIdentifier );
}

void ResourceAkonadi::Private::addresseeAdded( const KABC::Addressee &addressee, const QString &subResourceIdentifier )
{
  kDebug( 5700 ) << "Addressee (uid=" << addressee.uid()
                 << ", name=" << addressee.formattedName()
                 << "), subResource=" << subResourceIdentifier;

  mChanges.remove( addressee.uid() );

  // check if we already have it, i.e. if it is the result of us saving it
  if ( mParent->mAddrMap.constFind( addressee.uid() ) == mParent->mAddrMap.constEnd() ) {
    Addressee addr = addressee;
    addr.setResource( mParent );
    mParent->mAddrMap.insert( addressee.uid(), addr );

    mUidToResourceMap.insert( addressee.uid(), subResourceIdentifier );

    if ( !isLoading() ) {
      mParent->addressBook()->emitAddressBookChanged();
    }
  }
}

void ResourceAkonadi::Private::addresseeChanged( const KABC::Addressee &addressee, const QString &subResourceIdentifier )
{
  kDebug( 5700 ) << "Addressee (uid=" << addressee.uid()
                 << ", name=" << addressee.formattedName()
                 << "), subResource=" << subResourceIdentifier;

  mChanges.remove( addressee.uid() );

  Addressee oldAddressee = mParent->mAddrMap[ addressee.uid() ];
  if ( oldAddressee == addressee ) {
    kDebug( 5700 ) << "No change to addressee data";
    return;
  }

  Addressee addr = addressee;
  addr.setResource( mParent );
  mParent->mAddrMap[ addressee.uid() ] = addr;

  if ( !isLoading() ) {
    mParent->addressBook()->emitAddressBookChanged();
  }
}

void ResourceAkonadi::Private::addresseeRemoved( const QString &uid, const QString &subResourceIdentifier )
{
  kDebug( 5700 ) << "Addressee (uid=" << uid
                 << "), subResource=" << subResourceIdentifier;

  mChanges.remove( uid );

  // check if we still have it, i.e. it is not a result of us deleting it
  if ( mParent->mAddrMap.constFind( uid ) != mParent->mAddrMap.constEnd() ) {
    mParent->mAddrMap.remove( uid );
    mUidToResourceMap.remove( uid );

    if ( !isLoading() ) {
      mParent->addressBook()->emitAddressBookChanged();
    }
  }
}

void ResourceAkonadi::Private::contactGroupAdded( const KABC::ContactGroup &contactGroup, const QString &subResourceIdentifier )
{
  kDebug( 5700 ) << "ContactGroup (uid=" << contactGroup.id()
                 << ", name=" << contactGroup.name()
                 << "), subResource=" << subResourceIdentifier;

  mChanges.remove( contactGroup.id() );

  // check if we already have it, i.e. if it is the result of us saving it
  if ( mParent->mDistListMap.constFind( contactGroup.id() ) == mParent->mDistListMap.constEnd() ) {
    // block scope for BoolGuard
    {
      BoolGuard internalChange( mInternalDataChange, true );
      (void)distListFromContactGroup( contactGroup );
    }

    mUidToResourceMap.insert( contactGroup.id(), subResourceIdentifier );

    if ( !isLoading() ) {
      mParent->addressBook()->emitAddressBookChanged();
    }
  }
}

void ResourceAkonadi::Private::contactGroupChanged( const KABC::ContactGroup &contactGroup, const QString &subResourceIdentifier )
{
  kDebug( 5700 ) << "ContactGroup (uid=" << contactGroup.id()
                 << ", name=" << contactGroup.name()
                 << "), subResource=" << subResourceIdentifier;

  mChanges.remove( contactGroup.id() );

  // TODO check if we can compare distribution lists and whether we
  // can do update instead of remove/add
  DistributionListMap::iterator findIt = mParent->mDistListMap.find( contactGroup.id() );
  if ( findIt == mParent->mDistListMap.end() ) {
    kError( 5700 ) << "No distribution list for changed contactgroup";
    contactGroupAdded( contactGroup, subResourceIdentifier );
    return;
  }

  // block scope for BoolGuard
  {
    BoolGuard internalChange( mInternalDataChange, true );
    DistributionList *distList = findIt.value();
    delete distList;
    distList = distListFromContactGroup( contactGroup );
  }

  if ( !isLoading() ) {
    mParent->addressBook()->emitAddressBookChanged();
  }
}

void ResourceAkonadi::Private::contactGroupRemoved( const QString &uid, const QString &subResourceIdentifier )
{
  kDebug( 5700 ) << "ContactGroup (uid=" << uid
                 << "), subResource=" << subResourceIdentifier;

  mChanges.remove( uid );

  // check if we still have it, i.e. it is not a result of us deleting it
  const DistributionListMap::const_iterator findIt = mParent->mDistListMap.constFind( uid );
  if ( findIt != mParent->mDistListMap.constEnd() ) {
    // block scope for BoolGuard
    {
      BoolGuard internalChange( mInternalDataChange, true );
      DistributionList *distList = findIt.value();
      delete distList;
    }
    mUidToResourceMap.remove( uid );

    if ( !isLoading() ) {
      mParent->addressBook()->emitAddressBookChanged();
    }
  }
}

DistributionList *ResourceAkonadi::Private::distListFromContactGroup( const ContactGroup &contactGroup ) const
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

#include "resourceakonadi_p.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
