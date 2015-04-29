/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "kolabresourcestate.h"
#include "kolabhelpers.h"
#include "kolabmessagehelper.h"

#include <imapresource.h>

#include <collectionannotationsattribute.h>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/CachePolicy>
#include <noselectattribute.h>
#include <Akonadi/KMime/MessageParts>
#include "kolabresource_debug.h"

KolabResourceState::KolabResourceState(ImapResource *resource, const TaskArguments &arguments)
    : ResourceState(resource, arguments)
{

}

static Akonadi::Collection processAnnotations(const Akonadi::Collection &collection)
{
    if (collection.attribute<Akonadi::CollectionAnnotationsAttribute>()) {
        Akonadi::Collection col = collection;
        const QMap<QByteArray, QByteArray> rawAnnotations = col.attribute<Akonadi::CollectionAnnotationsAttribute>()->annotations();
        const QByteArray type = KolabHelpers::getFolderTypeAnnotation(rawAnnotations);
        const Kolab::FolderType folderType = KolabHelpers::folderTypeFromString(type);
        col.setContentMimeTypes(KolabHelpers::getContentMimeTypes(folderType));

        const QString icon = KolabHelpers::getIcon(folderType);
        if (!icon.isEmpty()) {
            // qCDebug(KOLABRESOURCE_LOG) << " setting icon " << icon;
            Akonadi::EntityDisplayAttribute *attr = col.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
            attr->setIconName(icon);
        }
        if (folderType != Kolab::MailType) {
            //Groupware data always requires the full message, because it cannot translate without the body
            Akonadi::CachePolicy cachePolicy = col.cachePolicy();
            QStringList localParts = cachePolicy.localParts();
            if (!localParts.contains(QLatin1String(Akonadi::MessagePart::Body))) {
                localParts << QLatin1String(Akonadi::MessagePart::Body);
                cachePolicy.setLocalParts(localParts);
                cachePolicy.setCacheTimeout(-1);
                cachePolicy.setInheritFromParent(false);
                cachePolicy.setSyncOnDemand(true);
                col.setCachePolicy(cachePolicy);
            }
        }
        if (folderType == Kolab::ConfigurationType) {
            //we want to hide this folder from indexing and display, but still have the data available locally.
            col.setEnabled(false);
            col.setShouldList(Akonadi::Collection::ListSync, true);
        }
        if (!KolabHelpers::isHandledType(folderType)) {
            //If we don't handle the folder, make sure we don't download the messages
            col.attribute<NoSelectAttribute>(Akonadi::Entity::AddIfMissing);
        }
        return col;
    }
    return collection;
}

void KolabResourceState::collectionAttributesRetrieved(const Akonadi::Collection& collection)
{
    if (!collection.isValid() && collection.remoteId().isEmpty()) {
        ResourceState::collectionAttributesRetrieved(collection);
        return;
    }
    const Akonadi::Collection col = processAnnotations(collection);
    ResourceState::collectionAttributesRetrieved(col);
}

void KolabResourceState::collectionsRetrieved(const Akonadi::Collection::List &collections)
{
    Akonadi::Collection::List modifiedCollections;
    Q_FOREACH (const Akonadi::Collection &col, collections) {
        modifiedCollections << processAnnotations(col);
    }
    ResourceState::collectionsRetrieved(modifiedCollections);
}

MessageHelper::Ptr KolabResourceState::messageHelper() const
{
    return MessageHelper::Ptr(new KolabMessageHelper(collection()));
}
