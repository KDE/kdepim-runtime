/*
    Copyright (C) 2012    Frank Osterfeld <osterfeld@kde.org>

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

#ifndef OWNCLOUDRSSRESOURCE_H
#define OWNCLOUDRSSRESOURCE_H

#include "jobs.h"

#include <Akonadi/ResourceBase>

#include <QPointer>

namespace KWallet {
    class Wallet;
}
class OwncloudRssResource : public Akonadi::ResourceBase,
                            public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    explicit OwncloudRssResource( const QString &id );
    ~OwncloudRssResource();

public Q_SLOTS:
    void configure( WId windowId );

protected:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void collectionChanged( const Akonadi::Collection& collection );
    void collectionAdded( const Akonadi::Collection& collection, const Akonadi::Collection& parent );
    void collectionRemoved( const Akonadi::Collection& collection );

private Q_SLOTS:
    void foldersListed( KJob* );
    void feedsListed( KJob* );
    void walletOpened( bool success );

private:
    void waitForWalletAndStart( Job* );

private:
    void reallyConfigure( WId windowId );

private:
    KWallet::Wallet* m_wallet;
    bool m_walletOpenedReceived;
    QString m_password;
    QVector<Job*> m_pendingJobsWaitingForWallet;
    QVector<WId> m_configDialogsWaitingForWallet;
    QPointer<ListNodeJob> m_listJob;
    QVector<ListNodeJob::Node> m_folders;
};

#endif
