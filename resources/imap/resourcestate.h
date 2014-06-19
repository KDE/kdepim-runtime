/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef RESOURCESTATE_H
#define RESOURCESTATE_H

#include "resourcestateinterface.h"
#include "messagehelper.h"

class ImapResourceBase;

struct TaskArguments {
    TaskArguments(){}
    TaskArguments(const Akonadi::Item &_item): items(Akonadi::Item::List() << _item) {}
    TaskArguments(const Akonadi::Item &_item, const Akonadi::Collection &_collection): collection(_collection), items(Akonadi::Item::List() << _item) {}
    TaskArguments(const Akonadi::Item &_item, const QSet<QByteArray> &_parts): items(Akonadi::Item::List() << _item), parts(_parts) {}
    TaskArguments(const Akonadi::Item::List &_items): items(_items) {}
    TaskArguments(const Akonadi::Item::List &_items, const QSet<QByteArray> &_addedFlags, const QSet<QByteArray> &_removedFlags): items(_items), addedFlags(_addedFlags), removedFlags(_removedFlags) {}
    TaskArguments(const Akonadi::Item::List &_items, const Akonadi::Collection &_sourceCollection, const Akonadi::Collection &_targetCollection): items(_items), sourceCollection(_sourceCollection), targetCollection(_targetCollection){}
    TaskArguments(const Akonadi::Collection &_collection): collection(_collection){}
    TaskArguments(const Akonadi::Collection &_collection, const Akonadi::Collection &_parentCollection): collection(_collection), parentCollection(_parentCollection){}
    TaskArguments(const Akonadi::Collection &_collection, const Akonadi::Collection &_sourceCollection, const Akonadi::Collection &_targetCollection): collection(_collection), sourceCollection(_sourceCollection), targetCollection(_targetCollection){}
    TaskArguments(const Akonadi::Collection &_collection, const QSet<QByteArray> &_parts): collection(_collection), parts(_parts){}
    Akonadi::Collection collection;
    Akonadi::Item::List items;
    Akonadi::Collection parentCollection; //only used as parent of a collection
    Akonadi::Collection sourceCollection;
    Akonadi::Collection targetCollection;
    QSet<QByteArray> parts;
    QSet<QByteArray> addedFlags;
    QSet<QByteArray> removedFlags;
};

class ResourceState : public ResourceStateInterface
{
public:
  explicit ResourceState( ImapResourceBase *resource, const TaskArguments &arguments );

public:
  ~ResourceState();

  virtual QString userName() const;
  virtual QString resourceName() const;
  virtual QStringList serverCapabilities() const;
  virtual QList<KIMAP::MailBoxDescriptor> serverNamespaces() const;

  virtual bool isAutomaticExpungeEnabled() const;
  virtual bool isSubscriptionEnabled() const;
  virtual bool isDisconnectedModeEnabled() const;
  virtual int intervalCheckTime() const;

  virtual Akonadi::Collection collection() const;
  virtual Akonadi::Item item() const;
  virtual Akonadi::Item::List items() const;

  virtual Akonadi::Collection parentCollection() const;

  virtual Akonadi::Collection sourceCollection() const;
  virtual Akonadi::Collection targetCollection() const;

  virtual QSet<QByteArray> parts() const;
  virtual QSet<QByteArray> addedFlags() const;
  virtual QSet<QByteArray> removedFlags() const;

  virtual QString rootRemoteId() const;

  virtual void setIdleCollection( const Akonadi::Collection &collection );
  virtual void applyCollectionChanges( const Akonadi::Collection &collection );

  virtual void collectionAttributesRetrieved( const Akonadi::Collection &collection );

  virtual void itemRetrieved( const Akonadi::Item &item );

  virtual void itemsRetrieved( const Akonadi::Item::List &items );
  virtual void itemsRetrievedIncremental( const Akonadi::Item::List &changed, const Akonadi::Item::List &removed );
  virtual void itemsRetrievalDone();

  virtual void setTotalItems(int);

  virtual void itemChangeCommitted( const Akonadi::Item &item );
  virtual void itemsChangesCommitted(const Akonadi::Item::List& items);

  virtual void collectionsRetrieved( const Akonadi::Collection::List &collections );

  virtual void collectionChangeCommitted( const Akonadi::Collection &collection );

  virtual void changeProcessed();

  virtual void searchFinished( const QVector<qint64> &result, bool isRid = true );

  virtual void cancelTask( const QString &errorString );
  virtual void deferTask();
  virtual void restartItemRetrieval(Akonadi::Collection::Id col);
  virtual void taskDone();

  virtual void emitError( const QString &message );
  virtual void emitWarning( const QString &message );

  virtual void emitPercent( int percent );

  virtual void synchronizeCollection(Akonadi::Collection::Id);
  virtual void synchronizeCollectionTree();
  virtual void scheduleConnectionAttempt();

  virtual QChar separatorCharacter() const;
  virtual void setSeparatorCharacter( const QChar &separator );

  virtual void showInformationDialog( const QString &message, const QString &title, const QString &dontShowAgainName );

  virtual int batchSize() const;

  virtual MessageHelper::Ptr messageHelper() const;

private:
  ImapResourceBase *m_resource;
  const TaskArguments m_arguments;
};

#endif
