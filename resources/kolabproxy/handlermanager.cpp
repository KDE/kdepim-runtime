/*
  Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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
#include "handlermanager.h"
#include <collectionannotationsattribute.h>

Kolab::FolderType HandlerManager::getFolderType( const Akonadi::Collection& collection )
{
    if (Akonadi::CollectionAnnotationsAttribute *attr = collection.attribute<Akonadi::CollectionAnnotationsAttribute>() ) {
        return Kolab::folderTypeFromString( Kolab::getFolderTypeAnnotation(attr->annotations()));
    }
    return Kolab::MailType;
}

bool HandlerManager::isKolabFolder(const Akonadi::Collection &collection)
{
    return (getFolderType(collection) != Kolab::MailType);
}

bool HandlerManager::isHandledKolabFolder(const Akonadi::Collection& collection)
{
    return KolabHandler::hasHandler(getFolderType(collection));
}

KolabHandler::Ptr HandlerManager::getHandler(Akonadi::Entity::Id collectionId)
{
    return mMonitoredCollections.value(collectionId);
}

QString HandlerManager::imapResourceForCollection( Akonadi::Collection::Id id )
{
    return mResourceIdentifiers.value(id);
}

bool HandlerManager::registerHandlerForCollection( const Akonadi::Collection &imapCollection, Kolab::Version version )
{
    if (isHandledKolabFolder(imapCollection)) {
        KolabHandler::Ptr handler =
        KolabHandler::createHandler(getFolderType(imapCollection), imapCollection);
        if (handler) {
            handler->setKolabFormatVersion(version);
            mMonitoredCollections.insert(imapCollection.id(), handler);
            mResourceIdentifiers.insert(imapCollection.id(), imapCollection.resource());
            return true;
        }
    }
    return false;
}

void HandlerManager::removeFolder(Akonadi::Entity::Id imapCollection)
{
    mMonitoredCollections.remove(imapCollection);
}

bool HandlerManager::isMonitored(Akonadi::Entity::Id imapCollection) const
{
    return mMonitoredCollections.contains(imapCollection);
}

QList< Akonadi::Entity::Id > HandlerManager::monitoredCollections() const
{
    return mMonitoredCollections.keys();
}

void HandlerManager::clear()
{
    mMonitoredCollections.clear();
}
