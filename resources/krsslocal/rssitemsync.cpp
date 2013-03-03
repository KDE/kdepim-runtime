/*
    Copyright (C) 2008    Dmitry Ivanov <vonami@gmail.com>

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

#include "rssitemsync.h"

#include <krss/item.h>

#include <KDebug>

RssItemSync::RssItemSync( const Akonadi::Collection &collection, QObject *parent )
    : ItemSync( collection, parent )
    , m_synchronizeFlags( false )
{
}

void RssItemSync::setSynchronizeFlags( bool synchronizeFlags )
{
    m_synchronizeFlags = synchronizeFlags;
}

bool RssItemSync::synchronizeFlags() const
{
    return m_synchronizeFlags;
}

bool RssItemSync::updateItem( const Akonadi::Item &storedItem, Akonadi::Item &newItem )
{
    if ( storedItem.mimeType() != KRss::Item::mimeType() ||
        newItem.mimeType() != KRss::Item::mimeType() ) {
        kWarning() << "Either storedItem or newItem doesn't have mimetype \'application/rss+xml\'";
        kWarning() << "Remote id:" << storedItem.remoteId();
        return false;
    }

    if ( !storedItem.hasPayload() || !newItem.hasPayload() ) {
        kWarning() << "Either storedItem or newItem doesn't have payload";
        kWarning() << "Remote id:" << storedItem.remoteId();
        return false;
    }

    const KRss::Item newRssItem = newItem.payload<KRss::Item>();
    const int newHash = newRssItem.hash();
    const int storedHash = storedItem.payload<KRss::Item>().hash();

    if ( !newRssItem.guidIsHash() && storedHash != newHash ) {
        kDebug() << "The article's content is updated:" << newItem.remoteId();

	Akonadi::Item::Flags flags = storedItem.flags();
	if (storedItem.hasFlag( KRss::Item::flagRead() ) &&
	      !storedItem.hasFlag( KRss::Item::flagUpdated() )) {
	    // case when an item was marked as read by a client 
	    // and the content changes on the server
	    flags.insert( KRss::Item::flagUpdated() );
	}
	newItem.setFlags( flags );
        return true;
    }

    if ( m_synchronizeFlags ) {
        const bool readFlagsDiffer = storedItem.hasFlag( KRss::Item::flagRead() ) !=
                                     newItem.hasFlag( KRss::Item::flagRead() );
        const bool importantFlagsDiffer = storedItem.hasFlag( KRss::Item::flagImportant() ) !=
                                          newItem.hasFlag( KRss::Item::flagImportant() );

        // either 'Seen' or 'Important' was changed in the backend
        // push the item to Akonadi clearing 'New'
        if ( readFlagsDiffer || importantFlagsDiffer ) {
            kDebug() << "The article's flags are updated:" << newItem.remoteId();
            // We need to explicitely overwrite the item's flags
            // otherwise Akonadi::ItemModifyJob just ignores them,
            // see Akonadi::ItemModifyJob::doStart()
            newItem.setFlags( newItem.flags() );
            return true;
        }
    }

    return false;
}
