/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#include <Akonadi/AgentBase>
#include <Akonadi/Collection>
#include <Akonadi/Item>


/**
 * This agent dispatches mail put into the outbox collection.
 */
class MailDispatcherAgent : public Akonadi::AgentBase
{
  Q_OBJECT

  public:
    MailDispatcherAgent( const QString &id );
    ~MailDispatcherAgent();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  Q_SIGNALS:
    /**
      Emitted when the MDA has attempted to send an item.
     */
    void itemProcessed( const Akonadi::Item &item, bool result );

  protected:
    virtual void doSetOnline( bool online );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void dispatch() )
    Q_PRIVATE_SLOT( d, void itemFetched( Akonadi::Item& ) )
    Q_PRIVATE_SLOT( d, void sendResult( KJob* ) )

};

#endif // MAILDISPATCHERAGENT_H
