/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef OUTBOXQUEUE_H
#define OUTBOXQUEUE_H

#include <QObject>

#include <Akonadi/Collection>
#include <Akonadi/Item>

class KJob;


/**
  Monitors the outbox collection and provides a queue of messages for the MDA to send.
 */
class OutboxQueue : public QObject
{
  Q_OBJECT
  friend class MailDispatcherAgent;

  public:
    // TODO docu
    explicit OutboxQueue( QObject *parent = 0 );
    virtual ~OutboxQueue();

    bool isEmpty() const;
    int count() const;
    /**
      Returns the size (in bytes) of all items in the queue.
    */
    qulonglong totalSize() const;

    /**
      Fetches an item and emits itemReady() when done.
    */
    void fetchOne();

  signals:
    void itemReady( Akonadi::Item &item );
    void newItems();

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void checkFuture() )
    Q_PRIVATE_SLOT( d, void collectionFetched( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemFetched( KJob* ) )
    Q_PRIVATE_SLOT( d, void outboxChanged() )
    Q_PRIVATE_SLOT( d, void itemAdded( Akonadi::Item ) )
    Q_PRIVATE_SLOT( d, void itemChanged( Akonadi::Item ) )
    Q_PRIVATE_SLOT( d, void itemMoved( Akonadi::Item, Akonadi::Collection, Akonadi::Collection ) )
    Q_PRIVATE_SLOT( d, void itemRemoved( Akonadi::Item ) )
    Q_PRIVATE_SLOT( d, void itemProcessed( Akonadi::Item, bool ) )

};


#endif
