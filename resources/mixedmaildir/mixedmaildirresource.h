/*  This file is part of the KDE project
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef MIXEDMAILDIR_RESOURCE_H
#define MIXEDMAILDIR_RESOURCE_H

#include <akonadi/resourcebase.h>

class MixedMaildirStore;

class MixedMaildirResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    MixedMaildirResource( const QString &id );
    ~MixedMaildirResource();

  public Q_SLOTS:
    void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    void aboutToQuit();

    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &dest );
    void itemRemoved( const Akonadi::Item &item );

    void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    void collectionChanged( const Akonadi::Collection &collection );
    void collectionChanged( const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes );
    void collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &dest );
    void collectionRemoved( const Akonadi::Collection &collection );

  private Q_SLOTS:
    void ensureDirExists();
    bool ensureSaneConfiguration();

    void retrieveCollectionsResult( KJob *job );
    void retrieveItemsResult( KJob *job );
    void retrieveItemResult( KJob *job );

    void itemAddedResult( KJob *job );
    void itemChangedResult( KJob *job );
    void itemMovedResult( KJob *job );
    void itemRemovedResult( KJob *job );

    void collectionAddedResult( KJob *job );
    void collectionChangedResult( KJob *job );
    void collectionMovedResult( KJob *job );
    void collectionRemovedResult( KJob *job );

    void compactStore( const QVariant &arg );
    void compactStoreResult( KJob *job );

  private:
    MixedMaildirStore *mStore;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
