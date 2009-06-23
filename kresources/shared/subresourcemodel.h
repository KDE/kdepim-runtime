/*
    This file is part of kdepim.
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

#ifndef KRES_AKONADI_SUBRESOURCEMODEL_H
#define KRES_AKONADI_SUBRESOURCEMODEL_H

#include "abstractsubresourcemodel.h"
#include "subresourcebase.h"

#include <akonadi/item.h>
#include <akonadi/collection.h>
#include <akonadi/mimetypechecker.h>

#include <QtCore/QHash>
#include <QtCore/QSet>

template <class SubResourceClass>
class SubResourceModel : public AbstractSubResourceModel
{
  public:
    typedef QHash<Akonadi::Collection::Id, SubResourceClass*> SubResByColId;
    typedef QHash<QString, SubResourceClass*> SubResByKResId;
    typedef QSet<Akonadi::Collection::Id> ColIdSet;
    typedef QHash<Akonadi::Item::Id, ColIdSet> ColIdsByItemId;

    explicit SubResourceModel( QObject *parent )
      : AbstractSubResourceModel( SubResourceClass::supportedMimeTypes(), parent )
    {
    }

    void writeConfig( KConfigGroup &config ) const
    {
      foreach ( const SubResourceClass *subResource, mSubResourcesByColId ) {
        subResource->writeConfig( config );
      }
    }

    SubResourceClass *subResource( Akonadi::Collection::Id colId ) const
    {
      return mSubResourcesByColId.value( colId, 0 );
    }

    SubResourceClass *subResource( const QString &kresId ) const
    {
      return mSubResourcesByKResId.value( kresId, 0 );
    }

    SubResourceBase *subResourceBase( Akonadi::Collection::Id colId ) const
    {
      return subResource( colId );
    }

    QList<SubResourceClass*> writableSubResourcesForMimeType( const QString &mimeType ) const
    {
      Akonadi::MimeTypeChecker mimeChecker;
      mimeChecker.addWantedMimeType( mimeType );

      QList<SubResourceClass*> result;
      foreach ( SubResourceClass *subResource, mSubResourcesByColId ) {
        if ( subResource->isWritable() && mimeChecker.isWantedCollection( subResource->collection() ) ) {
          result << subResource;
        }
      }

      return result;
    }

    QList<const SubResourceBase*> writableSubResourceBasesForMimeType( const QString &mimeType ) const
    {
      Akonadi::MimeTypeChecker mimeChecker;
      mimeChecker.addWantedMimeType( mimeType );

      QList<const SubResourceBase*> result;
      foreach ( const SubResourceClass *subResource, mSubResourcesByColId ) {
        if ( subResource->isWritable() && mimeChecker.isWantedCollection( subResource->collection() ) ) {
          result << subResource;
        }
      }

      return result;
    }

    SubResourceClass *findSubResourceForMappedItem( const QString &kresId ) const
    {
      foreach ( SubResourceClass *subResource, mSubResourcesByColId ) {
        if ( subResource->hasMappedItem( kresId ) ) {
          return subResource;
        }
      }

      return 0;
    }

  protected:
    SubResByColId mSubResourcesByColId;
    SubResByKResId mSubResourcesByKResId;
    ColIdsByItemId mCollectionsByItemId;

  protected:
    void clearModel()
    {
      qDeleteAll( mSubResourcesByColId );
      mSubResourcesByColId.clear();
      mSubResourcesByKResId.clear();
      mCollectionsByItemId.clear();
    }

  protected:
    void collectionAdded( const Akonadi::Collection &collection )
    {
      if ( mSubResourcesByColId.value( collection.id(), 0 ) == 0 ) {
        SubResourceClass *subResource = new SubResourceClass( collection );

        mSubResourcesByColId.insert( collection.id(), subResource );
        mSubResourcesByKResId.insert( subResource->subResourceIdentifier(), subResource );
        mSubResourceIdentifiers.insert( subResource->subResourceIdentifier() );

        emit subResourceAdded( subResource );
      } else
        collectionChanged( collection );
    }

    void collectionChanged( const Akonadi::Collection &collection )
    {
      SubResourceClass *subResource = mSubResourcesByColId.value( collection.id(), 0 );
      if ( subResource != 0 ) {
        subResource->changeCollection( collection );
      } else {
        collectionAdded( collection );
      }
    }

    void collectionRemoved( const Akonadi::Collection &collection )
    {
      SubResourceClass *subResource = mSubResourcesByColId.take( collection.id() );
      if ( subResource == 0 ) {
        return;
      }

      mSubResourcesByKResId.remove( subResource->subResourceIdentifier() );
      mSubResourceIdentifiers.remove( subResource->subResourceIdentifier() );

      emit subResourceRemoved( subResource );

      ColIdsByItemId::iterator it    = mCollectionsByItemId.begin();
      ColIdsByItemId::iterator endIt = mCollectionsByItemId.end();
      while ( it != endIt ) {
        ColIdSet colIds = it.value();
        colIds.remove( collection.id() );

        if ( colIds.isEmpty() ) {
          it = mCollectionsByItemId.erase( it );
        } else {
          ++it;
        }
      }

      delete subResource;
    }

    void itemAdded( const Akonadi::Item &item,
                    const Akonadi::Collection &collection )
    {
      SubResourceClass *subResource = mSubResourcesByColId.value( collection.id(), 0 );
      if ( subResource == 0 ) {
        kWarning( 5650 ) << "Item id=" << item.id()
                        << ", remoteId=" << item.remoteId()
                        << ", mimeType=" << item.mimeType()
                        << "added to an unknown collection"
                        << "(id=" << collection.id()
                        << ", remoteId=" << collection.remoteId()
                        << ")";
      } else {
        subResource->addItem( item );

        // insert if not present yet
        mCollectionsByItemId[ item.id() ].insert( collection.id() );
      }
    }

    void itemChanged( const Akonadi::Item &item )
    {
      const ColIdSet colIds = mCollectionsByItemId.value( item.id() );
      foreach ( const Akonadi::Collection::Id id, colIds ) {
        SubResourceClass *subResource = mSubResourcesByColId.value( id, 0 );
        Q_ASSERT( subResource != 0 );

        subResource->changeItem( item );
      }
    }

    void itemRemoved( const Akonadi::Item &item )
    {
      ColIdsByItemId::iterator findIt = mCollectionsByItemId.find( item.id() );
      foreach ( const Akonadi::Collection::Id id, findIt.value() ) {
        SubResourceClass *subResource = mSubResourcesByColId.value( id, 0 );
        Q_ASSERT( subResource != 0 );

        subResource->removeItem( item );
      }

      mCollectionsByItemId.erase( findIt );
    }
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
