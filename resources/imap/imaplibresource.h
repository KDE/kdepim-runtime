/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

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

#ifndef __IMAPLIB_RESOURCE_H__
#define __IMAPLIB_RESOURCE_H__

class Imaplib;

#include <resourcebase.h>

class ImaplibResource : public Akonadi::ResourceBase
{
    Q_OBJECT

public:
    ImaplibResource( const QString &id );
    ~ImaplibResource();

public Q_SLOTS:
    virtual void configure( WId windowId );

protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col, const QStringList &parts );
    bool retrieveItem( const Akonadi::Item &item, const QStringList &parts );

protected Q_SLOTS:
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QStringList &parts );
    virtual void itemRemoved( const Akonadi::DataReference &ref );

    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionRemoved( int id, const QString &remoteId );

private Q_SLOTS:
    void slotLogin( Imaplib* connection );
    void slotLoginFailed( Imaplib* connection );
    void slotAlert( Imaplib*, const QString& message );
    void slotMessagesInFolder( Imaplib*, const QString&, int );
    void slotFolderListReceived( const QStringList& list );
    void slotUidsAndFlagsReceived( Imaplib*, const QString&, const QStringList& );
    void slotHeadersReceived( Imaplib*,const QString&,const QStringList& );
    void slotMessageReceived( Imaplib*, const QString& mb, int uid, const QString& body );
    void slotSaveDone();

private:
    Imaplib* m_imap;
    QHash<QString, QString> m_flagsCache;
    QHash<QString, Akonadi::Item> m_itemCache;
    QHash<QString, int> m_amountMessagesCache;
    QString m_server;
    QString m_username;
    void connections();
    void manualAuth( Imaplib* connection, const QString& username );
    void startConnect();
    bool m_retrieveItemsRequested;
};

#endif
