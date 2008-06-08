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

#include <akonadi/resourcebase.h>

class ImaplibResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.Imaplib.Resource" )

public:
    ImaplibResource( const QString &id );
    ~ImaplibResource();

public Q_SLOTS:
    virtual void configure( WId windowId );
    void slotPurge( const QString& );

protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

protected:
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionRemoved( const Akonadi::Collection &collection );

private Q_SLOTS:
    void slotLogin( Imaplib* connection );
    void slotLoginFailed( Imaplib* connection );
    void slotAlert( Imaplib*, const QString& message );
    void slotMessagesInFolder( Imaplib*, const QString&, int );
    void slotFolderListReceived( const QStringList& list, const QStringList& );
    void slotUidsAndFlagsReceived( Imaplib*, const QString&, const QStringList& );
    void slotHeadersReceived( Imaplib*,const QString&,const QStringList& );
    void slotMessageReceived( Imaplib*, const QString& mb, int uid, const QString& body );
    void slotSaveDone( int );
    void slotCollectionAdded( bool success );
    void slotCollectionRemoved( bool );
    void slotIntegrity( const QString& mb, int totalShouldBe,
                        const QString& uidvalidity, const QString& uidnext );


private:
    Imaplib* m_imap;
    Akonadi::Collection m_collection;
    Akonadi::Item m_itemAdded;
    QHash<QString, QString> m_flagsCache;
    QHash<QString, Akonadi::Item> m_itemCache;
    QString m_server;
    QString m_username;
    void connections();
    void manualAuth( Imaplib* connection, const QString& username );
    void startConnect();
};

#endif
