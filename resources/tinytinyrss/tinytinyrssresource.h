/*
    Copyright (C) 2013    Frank Osterfeld <osterfeld@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TINYTINYRSSRESOURCE_H
#define TINYTINYRSSRESOURCE_H

#include "jobs.h"

#include <Akonadi/ResourceBase>

#include <QPointer>

namespace KWallet {
    class Wallet;
}

class TinyTinyRssResource : public Akonadi::ResourceBase,
                            public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    explicit TinyTinyRssResource( const QString &id );
    ~TinyTinyRssResource();

public Q_SLOTS:
    void configure( WId windowId );

protected:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void collectionChanged( const Akonadi::Collection& collection );
    void collectionAdded( const Akonadi::Collection& collection, const Akonadi::Collection& parent );
    void collectionRemoved( const Akonadi::Collection& collection );
    void itemChanged( const Akonadi::Item& item, const QSet<QByteArray>& partIdentifiers );

private Q_SLOTS:
    void feedCreated( KJob* );
    void retrieveCollectionsDone( KJob* );
    void retrieveItemsDone( KJob* );
    void retrieveSingleItemDone( KJob* );
    void collectionDeleted( KJob* );
    void folderCreated( KJob* );
    void walletOpened( bool success );
    void loginFinished( KJob* job );
    void articlesUpdated( KJob* );

private:
    void reallyConfigure( WId windowId );
    void setupJob( TransferJob* );
    void startLogin();
    void sendLogout();
    bool isLoggedIn() const;
    void ensureReady();
    bool isConfigured() const;

private:
    enum State {
        LoggedOff,
        LoggingIn,
        LoggedIn,
        LoggingOff
    };

private:
    State m_state;
    KWallet::Wallet* m_wallet;
    bool m_walletOpenedReceived;
    bool m_loginRequested;
    QString m_password;
    QString m_sessionId;
    QVector<WId> m_configDialogsWaitingForWallet;
};

#endif
