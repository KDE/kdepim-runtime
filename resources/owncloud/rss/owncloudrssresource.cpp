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

#include "owncloudrssresource.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "configdialog.h"

#include <QtDBus/QDBusConnection>

#include <KWindowSystem>

#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/AttributeFactory>

#include <KRss/Item>

using namespace Akonadi;
using namespace boost;

static QString mimeType() {
    return QLatin1String("application/rss+xml");
}

OwncloudRssResource::OwncloudRssResource( const QString &id )
    : ResourceBase( id )
{
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

#if 0
    m_policy.setInheritFromParent( false );
    m_policy.setSyncOnDemand( false );
    m_policy.setLocalParts( QStringList() << KRss::Item::HeadersPart << KRss::Item::ContentPart << Akonadi::Item::FullPayload );

    // Change recording makes the resource unusable for hours here
    // after migrating 130000 items, so disable it, as we don't write back item changes anyway.
    changeRecorder()->setChangeRecordingEnabled( false );
    changeRecorder()->fetchCollection( true );
    changeRecorder()->fetchChangedOnly( true );
    changeRecorder()->setItemFetchScope( ItemFetchScope() );
#endif
}

OwncloudRssResource::~OwncloudRssResource()
{
}

void OwncloudRssResource::retrieveCollections()
{
    // create a top-level collection
    Akonadi::Collection top;
    top.setParent( Collection::root() );
    //top.setRemoteId( opmlPath );
    top.setContentMimeTypes( QStringList() << Collection::mimeType() << mimeType() );
    top.setName( i18n("Owncloud News") );
    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( i18n("Owncloud News") );
    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setIconName( QString("application-opml+xml") );
    //TODO: modify CMakeLists.txt so that it installs the icon

    collectionsRetrieved( Akonadi::Collection::List() << top );
}


void OwncloudRssResource::retrieveItems( const Akonadi::Collection &collection )
{
    itemsRetrieved( Akonadi::Item::List() );
}



bool OwncloudRssResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
    Q_UNUSED( parts );

    itemRetrieved( item );
    return true;
}

void OwncloudRssResource::configure( WId windowId )
{

    QPointer<ConfigDialog> dlg( new ConfigDialog( identifier() ) );
    if ( windowId )
      KWindowSystem::setMainWindow( dlg, windowId );
    if ( dlg->exec() == KDialog::Accepted ) {
      emit configurationDialogAccepted();
    } else {
      emit configurationDialogRejected();
    }
    delete dlg;

    Settings::self()->writeConfig();

    synchronizeCollectionTree();
}

void OwncloudRssResource::collectionChanged(const Akonadi::Collection& collection)
{
    changeCommitted( collection );
}

void OwncloudRssResource::collectionAdded( const Collection &collection, const Collection &parent )
{
    Q_UNUSED( parent )
    changeCommitted( collection );
}

void OwncloudRssResource::collectionRemoved( const Collection &collection )
{
    changeCommitted( collection );
}


AKONADI_RESOURCE_MAIN( OwncloudRssResource )

#include "owncloudrssresource.moc"
