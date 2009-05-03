/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#ifndef MAILDISPATCHERAGENT_H
#define MAILDISPATCHERAGENT_H

#include <akonadi/agentbase.h>
#include <akonadi/item.h>

class KJob;


/**
 * This agent dispatches mail put into the outbox collection.
 */
class MailDispatcherAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    MailDispatcherAgent( const QString &id );
    ~MailDispatcherAgent();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected:
    virtual void doSetOnline( bool online );

    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionRemoved( const Akonadi::Collection &collection );
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers);
    virtual void itemRemoved( const Akonadi::Item &item );

  private:
    class Private;
    Private* const d;
    QMap < KJob *, Akonadi::Item > sentItems;

  private slots:
    void itemFetchDone( KJob *job );
    void transportResult( KJob *job );

};

#endif // MAILDISPATCHERAGENT_H
