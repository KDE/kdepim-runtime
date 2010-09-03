/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef CONTACTSRESOURCE_H
#define CONTACTSRESOURCE_H

#include <akonadi/resourcebase.h>

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kabc/contactgrouptool.h>
#include <kabc/vcardconverter.h>

class QDir;

class ContactsResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    ContactsResource( const QString &id );
    ~ContactsResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );
    virtual void aboutToQuit();

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &collection );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionRemoved( const Akonadi::Collection &collection );

    virtual void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                            const Akonadi::Collection &collectionDestination );
    virtual void collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource,
                                  const Akonadi::Collection &collectionDestination );

  private:
    Akonadi::Collection::List createCollectionsForDirectory( const QDir &parentDirectory,
                                                             const Akonadi::Collection &parentCollection ) const;
    QString baseDirectoryPath() const;
    void initializeDirectory( const QString &path ) const;
    Akonadi::Collection::Rights supportedRights( bool isResourceCollection ) const;
    QString directoryForCollection( const Akonadi::Collection& collection ) const;

  private:
    QStringList mSupportedMimeTypes;
};

#endif
