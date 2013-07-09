/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef DIRRESOURCE_H
#define DIRRESOURCE_H

#include "dirresourcebase.h"
#include "settings.h"

#include <KLocalizedString>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>

#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchScope>


template <typename Payload>
class DirResource : public DirResourceBase
{
  public:
    explicit DirResource( const QString &id )
        : DirResourceBase( id )
    {
    }

    virtual ~DirResource()
    {
        clear();
    }

  protected:
    virtual bool writeToFile( const Payload &payload ) const = 0;
    virtual Payload readFromFile( const QString &filePath ) const = 0;
    virtual QString payloadId( const Payload &payload ) const = 0;
    virtual bool isEmpty( const Payload &payload ) const = 0;

    void clear()
    {
        mItems.clear();
        mPaths.clear();
    }

    QString loadEntity( const QString &filePath )
    {
        mDirWatch->stopScan();
        Payload payload = readFromFile( filePath );
        if ( !isEmpty( payload ) ) {

            mIgnoredPaths << filePath;
            mDirWatch->startScan( true );

            const QString uid = payloadId( payload );
            mItems.insert( uid, payload );
            mPaths.insert( filePath, uid );

            return uid;
        }

        mDirWatch->startScan( true );

        kWarning() << "File" << filePath << "can't be loaded";
        return QString();
    }

    bool retrieveItem( const Akonadi::Item& item, const QSet< QByteArray >& parts )
    {
        const QString remoteId = item.remoteId();
        if ( !mItems.contains( remoteId ) ) {
            emit error( i18n( "Item with UID '%1' not found.", remoteId ) );
            return false;
        }

        Akonadi::Item newItem( item );
        newItem.setPayload<Payload>( mItems.value( remoteId ) );
        newItem.setMimeType( mimeType() );
        itemRetrieved( newItem );

        return true;
    }

    void itemAdded( const Akonadi::Item& item, const Akonadi::Collection &collection )
    {
        storeItem( item );
    }

    void itemChanged( const Akonadi::Item& item, const QSet< QByteArray > &parts )
    {
        storeItem( item );
    }

    void itemRemoved( const Akonadi::Item &item )
    {
        if ( Settings::self()->readOnly() ) {
            emit error( i18n( "Trying to write to a read-only directory: '%1'", directoryName() ) );
            cancelTask();
            return;
        }

        // remove it from the cache...
        mItems.remove( item.remoteId() );

        // ... and remove it from the file system
        mDirWatch->stopScan();
        const QString filePath = directoryFileName( item.remoteId() );
        mPaths.remove( filePath );

        QFile::remove( filePath );
        mIgnoredPaths << filePath;
        mDirWatch->startScan();

        changeProcessed();
    }

    void storeItem( const Akonadi::Item &item )
    {
        if ( Settings::self()->readOnly() ) {
            emit error( i18n( "Trying to write to a read-only directory: '%1'", directoryName() ) );
            cancelTask();
            return;
        }

        Payload payload;
        if ( item.hasPayload<Payload>() ) {
            payload  = item.payload<Payload>();
        }

        if ( !isEmpty( payload ) ) {
            const QString uid = payloadId( payload );
            // add it to the cache.
            mItems.insert( uid, payload );

            // ... and write it through to the file system
            mDirWatch->stopScan();
            const QString fileName = directoryFileName( uid );
            bool success = writeToFile( payload );
            if ( success ) {
                mIgnoredPaths << fileName;
                mPaths.insert( fileName, uid );

                // report everything ok
                Akonadi::Item newItem( item );
                newItem.setRemoteId( uid );
                changeCommitted( newItem );
            } else {
                kWarning() << "Can't write into file" << fileName;
            }

            mDirWatch->startScan( true );
        } else {
            changeProcessed();
        }
    }

    void retrieveItems( const Akonadi::Collection &col )
    {
        Q_UNUSED( col );

        Akonadi::Item::List items;

        Q_FOREACH ( const Payload &payload, mItems ) {
            Akonadi::Item item;
            item.setRemoteId( payloadId( payload ) );
            item.setMimeType( mimeType() );
            items.append( item );
        }

        itemsRetrieved( items );
    }

    void slotFileCreated( const QString &path )
    {
        kDebug() << path;
        if ( !ensureReady( path, DirResourceBase::FileCreated ) ) {
            return;
        }

        const QString uid = loadEntity( path );
        if ( !uid.isEmpty() ) {
            Akonadi::Item item;
            item.setRemoteId( uid );
            item.setMimeType( mimeType() );
            item.setPayload<Payload>( mItems.value( uid ) );
            new Akonadi::ItemCreateJob( item, mCollection, this );
        }
    }

    void slotDirChanged( const QString &path )
    {
        kDebug() << path;
        QFileInfo fInfo( path );
        if ( !fInfo.isFile() ) {
            return;
        }

        if ( !ensureReady( path, DirResourceBase::FileModified ) ) {
            return;
        }

        const QString uid = loadEntity( path );
        // File modification does not cause any further notifications
        mIgnoredPaths.removeAll( mPaths.key( uid ) );

        if ( !uid.isEmpty() ) {
            Akonadi::Item item;
            item.setRemoteId( uid );
            item.setMimeType( mimeType() );
            item.setPayload<Payload>( mItems.value( uid ) );
            item.setParentCollection( mCollection );
            new Akonadi::ItemModifyJob( item, this );
        }
    }

    void slotFileDeleted( const QString &path )
    {
        kDebug() << path;
        if ( !ensureReady( path, DirResourceBase::FileDeleted ) ) {
            return;
        }

        if ( mPaths.contains( path ) ) {
            const QString uid = mPaths.value( path );
            mItems.remove( uid );
            Akonadi::Item item;
            item.setRemoteId( uid );
            item.setParentCollection( mCollection );

            new Akonadi::ItemDeleteJob( item, this );
        }
    }

  protected:
    QHash<QString /* uid */, Payload> mItems;
    QHash<QString /* file */, QString /* uid */> mPaths;

};

#endif // DIRRESOURCEBASE_H
