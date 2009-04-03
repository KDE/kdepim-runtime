/*
    Copyright (C) 2009 Omat Holding B.V. <info@omat.nl>

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

#include "microblog.h"
#include "configdialog.h"
#include "communication.h"
#include "settingsadaptor.h"
#include "statusitem.h"
#include "idattribute.h"

#include <kdebug.h>
#include <klocale.h>
#include <kwindowsystem.h>

#include <akonadi/collection.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/attributefactory.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/item.h>


using namespace Akonadi;

MicroblogResource::MicroblogResource( const QString &id )
        : ResourceBase( id ), m_comm( 0 )
{
    new SettingsAdaptor( Settings::self() );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
            Settings::self(), QDBusConnection::ExportAdaptors );
    AttributeFactory::registerAttribute<IdAttribute>();
    initComm();
}

MicroblogResource::~MicroblogResource()
{
    delete m_comm;
}

void MicroblogResource::initComm()
{
    delete m_comm;
    m_comm = 0;

    if ( Settings::self()->userName().isEmpty() || Settings::self()->password().isEmpty() )
        return;

    m_comm = new Communication( this );
    m_comm->setService( Settings::self()->service() );
    m_comm->setCredentials( Settings::self()->userName(),  Settings::self()->password() );
    connect( m_comm, SIGNAL( statusList( const QList<QByteArray> ) ),
             SLOT( slotStatusList( const QList<QByteArray> ) ) );

    synchronizeCollectionTree();
}

void MicroblogResource::retrieveCollections()
{
    QHash<QString,Collection> collections;

    // if there is no connection, don't continue.
    if ( !m_comm ) {
        kDebug() << "Tried to retrieve collection, but there is no connection";
        collectionsRetrieved( collections.values() );
        return;
    }

    Collection root;
    root.setName( i18n( "%1's microblog", Settings::self()->name() ) );
    root.setRemoteId( "microblog" );
    root.setContentMimeTypes( QStringList( Collection::mimeType() ) );
    Collection::Rights rights = Collection::ReadOnly;
    root.setRights( rights );

    CachePolicy policy;
    policy.setInheritFromParent( false );
    policy.setSyncOnDemand( true );
    policy.setIntervalCheckTime( -1 );
    root.setCachePolicy( policy );

    collections[ "rootfolderunique" ] = root;

    // for all the folders, inherit it from the parent.
    policy.setInheritFromParent( true );

    QStringList folders;
    folders << "home" << "replies" << "favorites" << "inbox" << "outbox";
    QStringList foldersI18n;
    foldersI18n << i18n( "Home" ) << i18n( "Replies" )
    << i18n( "Favorites" ) << i18n( "Inbox" ) << i18n( "Outbox" );
    QStringList contentTypes;
    contentTypes << "application/x-vnd.kde.microblog";

    for ( int i=0; i<5; ++i ) {
        Collection c;
        c.setRemoteId( folders.at( i ) );
        c.setContentMimeTypes( contentTypes );
        c.setName( foldersI18n.at( i ) );
        c.setParentRemoteId( "microblog" );
        c.setRights( Collection::ReadOnly );

        CachePolicy policy;
        policy.setInheritFromParent( false );
        policy.setSyncOnDemand( true );
        policy.setIntervalCheckTime( 5 );
        c.setCachePolicy( policy );

        collections[ folders.at( i )] = c;
    }

    collectionsRetrieved( collections.values() );
}

void MicroblogResource::retrieveItems( const Akonadi::Collection &collection )
{
    if ( !m_comm ) {
        itemsRetrievalDone();
        return;
    }

    m_collection = collection;

    // get only newer items, except for favorites, which does not allow that.
    int id = 0;
    if ( m_collection.hasAttribute( "id" ) && collection.remoteId() != "favorites" ) {
        IdAttribute* currentid = static_cast<IdAttribute*>( m_collection.attribute( "id" ) );
        id = currentid->id();
    }
    kDebug() << "Getting everything for " << collection.name() << "and id >" << id;

    m_comm->retrieveFolder( collection.remoteId(), id );
}

void MicroblogResource::slotStatusList( const QList<QByteArray> list )
{
    kDebug() << list.count() << "received for" << m_collection.name();
    if ( list.count() == 0 ) {
        itemsRetrievalDone();
        return;
    }

    IdAttribute* currentid;
    if ( m_collection.hasAttribute( "id" ) ) {
        currentid = static_cast<IdAttribute*>( m_collection.attribute( "id" ) );
    } else {
        currentid = new IdAttribute( 0 );
        m_collection.addAttribute( currentid );
    }

    Item::List messages;
    foreach( const QByteArray& status, list ) {
        Akonadi::Item item( -1 );
        StatusItem stat( status );
        item.setRemoteId( QString::number( stat.id() ) );
        if ( stat.id() > currentid->id() )
            currentid->setId( stat.id() );
        item.setMimeType( "application/x-vnd.kde.microblog" );
        item.setPayload<StatusItem>( stat );
        item.setSize( status.length() );
        messages.append( item );
    }

    new Akonadi::CollectionModifyJob( m_collection );
    itemsRetrievedIncremental( messages, Item::List() );
}

bool MicroblogResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray>& )
{
    itemRetrieved( item );
    return true;
}

void MicroblogResource::configure( WId windowId )
{
    ConfigDialog dlg;
    if ( windowId )
        KWindowSystem::setMainWindow( &dlg, windowId );
    dlg.exec();
    if ( !Settings::self()->name().isEmpty() )
        setName( i18n( "%1's microblog", Settings::self()->name() ) );
    initComm();
}

AKONADI_RESOURCE_MAIN( MicroblogResource )

#include "microblog.moc"


