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

#ifndef KRES_AKONADI_RESOURCEPRIVATE_H
#define KRES_AKONADI_RESOURCEPRIVATE_H

#include "resourceprivatebase.h"
#include "subresourcemodel.h"

template <class SubResourceClass>
class SharedResourcePrivate : public ResourcePrivateBase
{
  public:
    typedef SubResourceModel<SubResourceClass> SubResourceModelClass;

    SharedResourcePrivate( IdArbiter *idArbiter, QObject *parent )
      : ResourcePrivateBase( idArbiter, parent ),
        mModel( this )
    {
      connect( &mModel, SIGNAL( subResourceAdded( SubResourceBase* ) ),
               SLOT( subResourceAdded( SubResourceBase* ) ) );

      connect( &mModel, SIGNAL( loadingResult( bool, QString ) ),
               SLOT( loadingResult( bool, QString ) ) );
    }

    SharedResourcePrivate( const KConfigGroup &config, IdArbiter *idArbiter, QObject *parent )
      : ResourcePrivateBase( config, idArbiter, parent ),
        mModel( this )
    {
      connect( &mModel, SIGNAL( subResourceAdded( SubResourceBase* ) ),
               SLOT( subResourceAdded( SubResourceBase* ) ) );

      connect( &mModel, SIGNAL( loadingResult( bool, QString ) ),
               SLOT( loadingResult( bool, QString ) ) );
    }

    bool load()
    {
      mModel.stopMonitoring();
      if ( mModel.load() ) {
        mModel.startMonitoring();
        return true;
      }

      return false;
    }

    bool asyncLoad()
    {
      mModel.stopMonitoring();
      return mModel.asyncLoad();
    }

    QStringList subResourceIdentifiers() const
    {
      return mModel.subResourceIdentifiers();
    }

    SubResourceClass *subResource( const QString &id ) const
    {
      return mModel.subResource( id );
    }

  protected:
     SubResourceModelClass mModel;

  protected:
    void writeResourceConfig( KConfigGroup &config ) const
    {
      mModel.writeConfig( config );
    }

    void clearResource()
    {
      mModel.clear();
    }

    SubResourceClass *storeSubResourceForMimeType( const QString &mimeType ) const
    {
      Akonadi::Collection collection = storeCollectionForMimeType( mimeType );
      if ( collection.isValid() ) {
        return mModel.subResource( collection.id() );
      }

      return 0;
    }
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
