/*
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

#include "nepomuktagresource.h"

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>

#include <akonadi/cachepolicy.h>
#include <akonadi/changerecorder.h>
#include <akonadi/linkjob.h>

#include <nepomuk/tag.h>
using namespace Akonadi;

NepomukTagResource::NepomukTagResource( const QString &id )
        : ResourceBase( id )
{
    changeRecorder()->fetchCollection( true );
}

NepomukTagResource::~NepomukTagResource()
{
}

void NepomukTagResource::retrieveCollections()
{
    QHash<QString,Collection> collections;

    QStringList contentTypes;
    contentTypes << "message/rfc822" << Collection::mimeType();

    Collection root;
    root.setName( i18n( "Tags" ) );
    root.setRemoteId( "nepomuktags" );
    root.setContentMimeTypes( QStringList( Collection::mimeType() ) );
    Collection::Rights rights = Collection::CanCreateCollection;
    root.setRights( rights );

    CachePolicy policy;
    policy.setInheritFromParent( false );
    policy.setSyncOnDemand( true );
    policy.setIntervalCheckTime( -1 );
    root.setCachePolicy( policy );

    collections[ "rootfolderunique" ] = root;

    // for all the folders, inherit it from the parent.
    policy.setInheritFromParent( true );

    QList<Nepomuk::Tag> tags = Nepomuk::Tag::allTags();
    foreach( const Nepomuk::Tag& tag, tags ) {
        kDebug() << "Found Nepomuk Tag:" << tag.genericLabel();
        if ( collections.contains( tag.genericLabel() ) )
            continue;
        Collection c;
        c.setName( tag.genericLabel() );
        c.setRemoteId( tag.genericLabel() );
        c.setRights( Collection::ReadOnly );
        c.setContentMimeTypes( contentTypes );
        c.setParentRemoteId( root.remoteId() );
        c.setCachePolicy( policy );
        collections[ tag.genericLabel()] = c;
    }
    collectionsRetrieved( collections.values() );
}

void NepomukTagResource::retrieveItems( const Akonadi::Collection & col )
{
    kDebug() << "Requested items for: " << col.remoteId();

    Item::List taggedMessages;
    Nepomuk::Tag tag( col.remoteId() );
    QList<Nepomuk::Resource> list = tag.tagOf();
    foreach( const Nepomuk::Resource& resource, list ) {
        if ( !resource.resourceUri().toString().startsWith( "akonadi:" ) )
            continue;
        kDebug() << "Found message: " << resource.resourceUri();
        taggedMessages << Item::fromUrl( KUrl( resource.resourceUri() ) );
    }

    kDebug() << "Messages found: " << taggedMessages.count();

    Akonadi::LinkJob* job = new Akonadi::LinkJob( col, taggedMessages, this );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotResult( KJob* ) ) );
}

void NepomukTagResource::slotResult( KJob* job )
{
    kDebug();
    if ( job->error() ) {
        kDebug() << job->errorString();
        emit error( job->errorString() );
    }
    itemsRetrievalDone();
}

void NepomukTagResource::collectionAdded( const Collection & collection, const Collection &parent )
{
    Q_UNUSED( parent );
    QString s = collection.name();
    Collection newCollection = collection;
    kDebug() << "Adding tag:" << s;

    // taken from kdelibs/nepomuk/core/ui/kmetadatatagwidget.cpp
    // Copyright (C) 2006-2007 Sebastian Trueg <trueg@kde.org>
    // ---
    bool exists = false;
    if ( !s.isEmpty() ) {
        // see if the tag exists
        QList<Nepomuk::Tag> l = Nepomuk::Tag::allTags();
        QListIterator<Nepomuk::Tag> tagIt( l );
        while ( tagIt.hasNext() ) {
            const Nepomuk::Tag& tag = tagIt.next();
            if ( tag.label() == s ||
                    tag.identifiers().contains( s ) ) {
                emit warning( i18n( "The tag %1 already exists", s ) );
                exists = true;
            }
        }
        if ( !exists ) {
            Nepomuk::Tag( s ).setLabel( s );
            newCollection.setRemoteId( s );
        }
    }
    // ---

    changeCommitted( newCollection );

    // TODO: sync folder list, as it does not seem to update automatically????
    synchronizeCollectionTree();
}

AKONADI_RESOURCE_MAIN( NepomukTagResource )

#include "nepomuktagresource.moc"


