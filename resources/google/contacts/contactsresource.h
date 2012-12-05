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

#ifndef GOOGLE_CONTACTS_CONTACTSRESOURCE_H
#define GOOGLE_CONTACTS_CONTACTSRESOURCE_H

#include <Akonadi/ResourceBase>
#include <Akonadi/AgentBase>
#include <Akonadi/Collection>
#include <Akonadi/Item>

#include <QQueue>

#include <libkgapi/common.h>
#include <libkgapi/account.h>

namespace KGAPI {
  class AccessManager;
  class Reply;
  class Request;
}

class QTimer;
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;

using namespace KGAPI;

class ContactsResource: public Akonadi::ResourceBase,
                        public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    ContactsResource( const QString &id );

    ~ContactsResource();

    using ResourceBase::synchronize;

  public Q_SLOTS:
    virtual void configure( WId windowID );

    void reloadConfig();

  protected Q_SLOTS:
    void retrieveCollections();

    void retrieveItems( const Akonadi::Collection &collection );
    bool retrieveItem( const Akonadi::Item &item, const QSet< QByteArray >& parts );

    void itemRemoved( const Akonadi::Item &item );
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet< QByteArray >& partIdentifiers );
    void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                    const Akonadi::Collection &collectionDestination );

  protected:
    void aboutToQuit();

  private Q_SLOTS:
    void error( KGAPI::Error errCode, const QString &msg );

    void slotAbortRequested();

    void initialItemsFetchJobFinished( KJob *job );
    void contactListReceived( KJob *job );

    void execFetchPhotoQueue();
    void photoRequestFinished( QNetworkReply *reply );

    void replyReceived( KGAPI::Reply *reply );

    void contactUpdated( KGAPI::Reply *reply );
    void contactCreated( KGAPI::Reply *reply );
    void contactRemoved( KGAPI::Reply *reply );

    void emitPercent( KJob *job, ulong progress );

  private:
    void abort();

    void updatePhoto( Akonadi::Item &item );
    void fetchPhoto( Akonadi::Item &item );

    Account::Ptr getAccount();

    KGAPI::Account::Ptr m_account;

    KGAPI::AccessManager *m_gam;

    QMap< QString, Akonadi::Collection > m_collections;

    QNetworkAccessManager *m_photoNam;
    QQueue< QNetworkRequest > m_fetchPhotoQueue;
    QTimer *m_fetchPhotoScheduler;
    int m_fetchPhotoScheduleInterval;
    int m_fetchPhotoBatchSize;
};

#endif // CONTACTSRESOURCE_H
