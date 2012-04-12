/*
 *  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 * 
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */


#include "upgradejob.h"
#include "kolabdefs.h"
#include "kolabhandler.h"
#include <collectionannotationsattribute.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemcreatejob.h>

using namespace Akonadi;

UpgradeJob::UpgradeJob(Kolab::Version targetVersion, const Akonadi::AgentInstance& instance, QObject* parent)
    : Akonadi::Job(parent),
    m_targetVersion(targetVersion),
    m_agentInstance(instance)
{
    kDebug() << targetVersion;
}

void UpgradeJob::doStart()
{
    kDebug();
    //Get all subdirectories of kolab resource
    CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
    job->fetchScope().setResource( m_agentInstance.identifier() );
    connect( job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)) );
}

void UpgradeJob::collectionFetchResult(KJob* job)
{
    if ( job->error() )
        return; // Akonadi::Job propagates that automatically
        
    foreach ( const Collection &col, static_cast<CollectionFetchJob*>( job )->collections() ) {
        kDebug() << "upgarding " << col.id();
        
        KolabV2::FolderType folderType = KolabV2::Mail;
        CollectionAnnotationsAttribute *attr = 0;
        if ( (attr = col.attribute<CollectionAnnotationsAttribute>()) ) {
            folderType = KolabV2::folderTypeFromString( attr->annotations().value( KOLAB_FOLDER_TYPE_ANNOTATION ) );
        }
        if (folderType == KolabV2::Mail) {
            kDebug() << "skipping mail folder";
            continue;
        }
        
        ItemFetchJob *itemFetchJob = new ItemFetchJob(col, this);
        itemFetchJob->fetchScope().fetchFullPayload(true);
        itemFetchJob->fetchScope().setCacheOnly(false);
        connect( job, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)) );
    }
}

void UpgradeJob::itemFetchResult(KJob* job)
{
    if ( job->error() ) {
        return; // Akonadi::Job propagates that automatically
    }
    ItemFetchJob *j = static_cast<ItemFetchJob*>( job );
    if (j->items().isEmpty()) {
        qWarning() << "no items fetched ";
        return;
    }
    
    const Collection imapCollection = j->items().first().parentCollection();
    if (!imapCollection.isValid()) {
        qWarning() << "invalid imap collection";
        return;
    }
    
    //we have only one handler per collection, so we can evaluate it only once
    KolabHandler::Ptr handler;
    const QByteArray &kolabType = KolabHandler::kolabTypeForMimeType(QStringList() << j->items().first().mimeType());
    handler = KolabHandler::createHandler(kolabType, imapCollection);
    handler->setKolabFormatVersion(m_targetVersion);
    
    const Akonadi::Item::List &translatedItems = handler->translateItems(j->items());
    foreach(const Akonadi::Item &tItem, translatedItems) {
        kDebug() << "updating item " << tItem.id();
        Item imapItem(handler->contentMimeTypes()[0]);
        handler->toKolabFormat( tItem, imapItem );
        
        ItemCreateJob *cjob = new ItemCreateJob(imapItem, imapCollection);
//         cjob->setProperty( KOLAB_ITEM, QVariant::fromValue( tItem ) );
//         cjob->setProperty( IMAP_COLLECTION, QVariant::fromValue( imapCollection ) );
//         connect( cjob, SIGNAL(result(KJob*)), SLOT(imapItemCreationResult(KJob*)) );
    }    
}
