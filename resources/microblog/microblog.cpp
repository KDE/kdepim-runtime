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
#include "settings.h"

#include <kdebug.h>
#include <klocale.h>
#include <kwindowsystem.h>

#include <akonadi/collection.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/item.h>


using namespace Akonadi;

MicroblogResource::MicroblogResource( const QString &id )
        : ResourceBase( id )
{
}

MicroblogResource::~MicroblogResource()
{
}

void MicroblogResource::retrieveCollections()
{
    QHash<QString,Collection> collections;

    Collection root;
    root.setName( i18n( "Microblog" ) );
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

    collectionsRetrieved( collections.values() );
}

void MicroblogResource::retrieveItems( const Akonadi::Collection& )
{
    itemsRetrievalDone();
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
    setName( Settings::self()->name() );
}

AKONADI_RESOURCE_MAIN( MicroblogResource )

#include "microblog.moc"


