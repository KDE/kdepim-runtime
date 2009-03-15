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

#ifndef KRES_AKONADI_ABSTRACTSUBRESOURCEMODEL_H
#define KRES_AKONADI_ABSTRACTSUBRESOURCEMODEL_H

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QtCore/QObject>
#include <QtCore/QSet>

namespace Akonadi {
  class MimeTypeChecker;
  class Monitor;
}

class ItemFetchAdapter;
class KJob;

typedef QSet<QString> StringIdSet;
class QStringList;
class SubResourceBase;

class AbstractSubResourceModel : public QObject
{
  Q_OBJECT

  public:
    AbstractSubResourceModel( const QStringList &supportedMimeTypes, QObject *parent );

    virtual ~AbstractSubResourceModel();

    QStringList subResourceIdentifiers() const;

    void startMonitoring();

    void stopMonitoring();

    void clear();

    bool load();

    bool asyncLoad();

  Q_SIGNALS:
    void subResourceAdded( SubResourceBase *subResource );

    void subResourceRemoved( SubResourceBase *subResource );

    void loadingResult( bool ok, const QString &errorString );

  protected:
    Akonadi::Monitor *mMonitor;
    Akonadi::MimeTypeChecker *mMimeChecker;

    StringIdSet mSubResourceIdentifiers;

  protected:
    virtual void clearModel() = 0;

    virtual void collectionAdded( const Akonadi::Collection &collection ) = 0;

    virtual void collectionChanged( const Akonadi::Collection &collection ) = 0;

    virtual void collectionRemoved( const Akonadi::Collection &collection ) = 0;

    virtual void itemAdded( const Akonadi::Item &item,
                            const Akonadi::Collection &collection ) = 0;

    virtual void itemChanged( const Akonadi::Item &item ) = 0;

    virtual void itemRemoved( const Akonadi::Item &item ) = 0;

  private:
    class AsyncLoadContext;
    AsyncLoadContext *mAsyncLoadContext;

  private Q_SLOTS:
    void monitorCollectionAdded( const Akonadi::Collection &parentCollection,
                                 const Akonadi::Collection &collection );

    void monitorCollectionChanged( const Akonadi::Collection &collection );

    void monitorCollectionRemoved( const Akonadi::Collection &collection );

    void monitorItemAdded( const Akonadi::Item &item,
                           const Akonadi::Collection &collection );

    void monitorItemChanged( const Akonadi::Item &item );

    void monitorItemRemoved( const Akonadi::Item &item );

    void asyncCollectionsReceived( const Akonadi::Collection::List &collections );

    void asyncCollectionsResult( KJob *job );

    void asyncItemsReceived( const Akonadi::Collection &collection, const Akonadi::Item::List &items );

    void asyncItemsResult( ItemFetchAdapter *fetcher, KJob *job );
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
