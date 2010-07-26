/*
    Copyright 2009 Sebastian Sauer <sebsauer@kdab.net>

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

#ifndef INVITATIONSAGENT_H
#define INVITATIONSAGENT_H

#include <KCalCore/MemoryCalendar>

#include <Akonadi/AgentBase>
#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/ItemCreateJob>

#include <QtCore/QObject>

class KJob;

namespace KCalCore {
  class ScheduleMessage;
}

class InvitationsAgent;
class InvitationsCollection;

class InvitationsAgentItem : public QObject
{
  Q_OBJECT

  public:
    InvitationsAgentItem(InvitationsAgent *a, const Akonadi::Item &originalItem);
    virtual ~InvitationsAgentItem();
    void add(const Akonadi::Item &newItem);

  private Q_SLOTS:
    void createItemResult( KJob *job );
    void fetchItemDone( KJob* );
    void modifyItemDone( KJob *job );

  private:
    InvitationsAgent *m_agent;
    const Akonadi::Item m_originalItem;
    QList<Akonadi::ItemCreateJob*> m_jobs;
};

class InvitationsAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    explicit InvitationsAgent( const QString &id );
    virtual ~InvitationsAgent();

    Akonadi::Collection collection();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  private Q_SLOTS:
    void initStart();
    void initDone( KJob *job = 0 );

  private:
    Akonadi::Item handleContent( const QString &vcal,
                                 const KCalCore::MemoryCalendar::Ptr &calendar,
                                 const Akonadi::Item &item );

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );

    /*
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers );
    virtual void itemRemoved( const Akonadi::Item &item );
    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionRemoved( const Akonadi::Collection &collection );

    virtual void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination );
    virtual void itemLinked( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemUnlinked( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination );
    virtual void collectionChanged( const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes );
    */

  private:
    QString m_resourceId;
    InvitationsCollection *m_invitationsCollection;
    Akonadi::Collection m_collection;
};

#endif // MAILDISPATCHERAGENT_H
