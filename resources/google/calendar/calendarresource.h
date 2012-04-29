/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef CALENDARRESOURCE_H
#define CALENDARRESOURCE_H

#include <Akonadi/AgentBase>
#include <Akonadi/ResourceBase>
#include <Akonadi/Item>
#include <Akonadi/Collection>

#include <libkgoogle/common.h>
#include <libkgoogle/account.h>

namespace KGoogle {
class AccessManager;
class Account;
class Reply;
class Request;
}

using namespace KGoogle;

class CalendarResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{

    Q_OBJECT
  public:
    CalendarResource( const QString &id );
    ~CalendarResource();

    void configure( WId windowId );


  public Q_SLOTS:
    void reloadConfig();
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &collection );
    bool retrieveItem( const Akonadi::Item &item, const QSet< QByteArray >& parts );

    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet< QByteArray >& partIdentifiers );
    void itemRemoved( const Akonadi::Item &item );
    void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination );

  protected:
    void aboutToQuit();

  private Q_SLOTS:
    void error( const KGoogle::Error, const QString & );
    void slotAbortRequested();

    void cachedItemsRetrieved( KJob *job );
    void replyReceived( KGoogle::Reply *reply );

    void itemsReceived( KJob *job );
    void itemReceived( KGoogle::Reply *reply );
    void itemCreated( KGoogle::Reply *reply );
    void itemUpdated( KGoogle::Reply *reply );
    void itemRemoved( KGoogle::Reply *reply );
    void itemMoved( KGoogle::Reply *reply );

    void taskListReceived( KJob *job );
    void calendarsReceived( KJob *job );

    /* The actual update of task */
    void taskDoUpdate( KGoogle::Reply *reply );

    void taskReceived( KGoogle::Reply *reply );
    void tasksReceived( KJob *job );
    void taskCreated( KGoogle::Reply *reply );
    void taskUpdated( KGoogle::Reply *reply );
    void taskRemoved( KGoogle::Reply *reply );

    void removeTaskFetchJobFinished( KJob *job );
    void doRemoveTask( KJob *job );

    void eventReceived( KGoogle::Reply *reply );
    void eventsReceived( KJob *job );
    void eventCreated( KGoogle::Reply *reply );
    void eventUpdated( KGoogle::Reply *reply );
    void eventRemoved( KGoogle::Reply *reply );
    void eventMoved( KGoogle::Reply *reply );

    void emitPercent( KJob *job, ulong percent );

  private:
    void abort();
    Account::Ptr getAccount();

    AccessManager *m_gam;

    Account::Ptr m_account;

    Akonadi::Collection::List m_collections;
    bool m_fetchedCalendars;
    bool m_fetchedTaskLists;
};

#endif // CALENDARRESOURCE_H
