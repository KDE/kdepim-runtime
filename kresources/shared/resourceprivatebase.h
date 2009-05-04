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

#ifndef KRES_AKONADI_RESOURCEPRIVATEBASE_H
#define KRES_AKONADI_RESOURCEPRIVATEBASE_H

#include <akonadi/collection.h>

#include "storeconfigiface.h"

#include <KConfigGroup>

#include <QtCore/QObject>

namespace Akonadi {
  class Item;
}

class AbstractSubResourceModel;
class IdArbiterBase;
class ItemSaveContext;
class StoreCollectionDialog;
class SubResourceBase;

class ResourcePrivateBase : public QObject, public StoreConfigIface
{
  Q_OBJECT

  public:
    enum State
    {
      Closed,
      Opened,
      Failed
    };

    enum ChangeType
    {
      NoChange,
      Added,
      Changed,
      Removed
    };

    typedef QHash<QString, ChangeType> ChangeByKResId;

    ResourcePrivateBase( IdArbiterBase *idArbiter, QObject *parent );

    ResourcePrivateBase( const KConfigGroup &config, IdArbiterBase *idArbiter, QObject *parent );

    virtual ~ResourcePrivateBase();

    bool doOpen();

    bool doClose();

    bool doLoad();

    bool doAsyncLoad();

    bool doSave();

    bool doAsyncSave();

    void writeConfig( KConfigGroup &config ) const;

    void clear();

    void setStoreCollection( const Akonadi::Collection& collection )
    {
      setDefaultStoreCollection( collection );
    }

    Akonadi::Collection storeCollection() const
    {
      return defaultStoreCollection();
    }

    void setStoreCollectionsByMimeType( const CollectionsByMimeType &collections );

    CollectionsByMimeType storeCollectionsByMimeType() const;

    void setDefaultStoreCollection( const Akonadi::Collection &collection );

    Akonadi::Collection defaultStoreCollection() const;

    bool addLocalItem( const QString &uid, const QString &mimeType );

    void changeLocalItem( const QString &uid );

    void removeLocalItem( const QString &uid );

  protected:
    KConfigGroup mConfig;

    IdArbiterBase *mIdArbiter;

    ChangeByKResId mChanges;

    Akonadi::Collection mDefaultStoreCollection;

    CollectionsByMimeType mStoreCollectionsByMimeType;

    QMap<QString, QString> mUidToResourceMap;

    StoreCollectionDialog *mStoreCollectionDialog;

  protected:
    State state() const;

    bool isLoading() const;

    bool startAkonadi();

    Akonadi::Collection storeCollectionForMimeType( const QString &mimeType ) const;

    bool prepareItemSaveContext( ItemSaveContext &saveContext );

    bool prepareItemSaveContext( const ChangeByKResId::const_iterator &it, ItemSaveContext &saveContext );

    virtual bool openResource() = 0;

    virtual bool closeResource() = 0;

    virtual bool loadResource() = 0;

    virtual bool asyncLoadResource() = 0;

    virtual void writeResourceConfig( KConfigGroup &config ) const = 0;

    virtual void clearResource() = 0;

    virtual const SubResourceBase *subResourceBase( const QString &subResourceIdentifier ) const = 0;

    virtual const SubResourceBase *findSubResourceForMappedItem( const QString &kresId ) const = 0;

    virtual const SubResourceBase *storeSubResourceForMimeType( const QString &mimeType ) const = 0;

    virtual QList<const SubResourceBase*> writableSubResourcesForMimeType( const QString &mimeType ) const = 0;

    virtual const SubResourceBase *storeSubResourceFromUser( const QString &kresId, const QString &mimeType ) = 0;

    virtual Akonadi::Item createItem( const QString &kresId ) = 0;

    virtual Akonadi::Item updateItem( const Akonadi::Item &item, const QString &kresId, const QString &originalId ) = 0;

    virtual const AbstractSubResourceModel *subResourceModel() const = 0;

  protected Q_SLOTS:
    virtual void subResourceAdded( SubResourceBase *subResource );

    virtual void subResourceRemoved( SubResourceBase *subResource );

    virtual void loadingResult( bool ok, const QString &errorString );

    virtual void savingResult( bool ok, const QString &errorString );

  private:
    State mState;
    bool  mLoadingInProgress;
};

class BoolGuard
{
  public:
    BoolGuard( bool &variable, bool overrideValue )
      : mVariable( variable ), mBaseValue( variable )
    {
      mVariable = overrideValue;
    }

    ~BoolGuard()
    {
      mVariable = mBaseValue;
    }

  protected:
    bool &mVariable;
    const bool mBaseValue;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
