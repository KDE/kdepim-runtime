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

#include <KConfigGroup>

#include <QtCore/QHash>
#include <QtCore/QObject>

class IdArbiterBase;
class ItemSaveContext;
class SubResourceBase;

class ResourcePrivateBase : public QObject
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

    bool doSave();

    bool doAsyncSave();

    void writeConfig( KConfigGroup &config ) const;

    void clear();

  Q_SIGNALS:
    void savingError( const QString &errorString );

  protected:
    KConfigGroup mConfig;

    IdArbiterBase *mIdArbiter;

    ChangeByKResId mChanges;

    Akonadi::Collection mDefaultStoreCollection;

  protected:
    State state() const;

    bool startAkonadi();

    Akonadi::Collection storeCollectionForMimeType( const QString &mimeType ) const;

    virtual bool openResource() = 0;

    virtual bool closeResource() = 0;

    virtual bool prepareItemSaveContext( ItemSaveContext &saveContext ) = 0;

    virtual void writeResourceConfig( KConfigGroup &config ) const = 0;

    virtual void clearResource() = 0;

  protected Q_SLOTS:
    virtual void subResourceAdded( SubResourceBase *subResource );

    virtual void subResourceRemoved( SubResourceBase *subResource );

    virtual void loadingResult( bool ok, const QString &errorString );

    virtual void savingResult( bool ok, const QString &errorString );

  private:
    State mState;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
