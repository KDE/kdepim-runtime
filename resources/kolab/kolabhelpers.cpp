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

#include "kolabhelpers.h"
#include <KMime/KMimeMessage>
#include <KCalCore/Incidence>
#include <Akonadi/Collection>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <akonadi/notes/noteutils.h>
#include <kolabobject.h>
#include <errorhandler.h>

bool KolabHelpers::checkForErrors(const Akonadi::Item &item)
{
    if (!Kolab::ErrorHandler::instance().errorOccured()) {
        Kolab::ErrorHandler::instance().clear();
        return false;
    }

    QString errorMsg;
    foreach (const Kolab::ErrorHandler::Err &error, Kolab::ErrorHandler::instance().getErrors()) {
        errorMsg.append(error.message);
        errorMsg.append(QLatin1String("\n"));
    }

    kWarning() << "Error on item with id: " << item.id() << " remote id: " << item.remoteId() << ":\n" << errorMsg;
    Kolab::ErrorHandler::instance().clear();
    return true;
}

Akonadi::Item KolabHelpers::translateFromImap(Kolab::FolderType folderType, const Akonadi::Item &imapItem, bool &ok)
{
    //Avoid trying to convert imap messages
    if (folderType == Kolab::MailType) {
        return imapItem;
    }

    //No payload, so it's a flag change. We ignore flag changes on groupware data.
    if (!imapItem.hasPayload()) {
        ok = false;
        return Akonadi::Item();
    }
    if (!imapItem.hasPayload<KMime::Message::Ptr>()) {
        kWarning() << "Payload is not a MessagePtr!";
        Q_ASSERT(false);
        ok = false;
        return Akonadi::Item();
    }

    const KMime::Message::Ptr payload = imapItem.payload<KMime::Message::Ptr>();
    const Kolab::KolabObjectReader reader(payload);
    if (checkForErrors(imapItem)) {
        //By not delivering items we cannot translate, they are simply deleted from local storage
        ok = false;
        return Akonadi::Item();
    }
    switch (reader.getType()) {
        case Kolab::EventObject:
        case Kolab::TodoObject:
        case Kolab::JournalObject:
        {
            const KCalCore::Incidence::Ptr incidencePtr = reader.getIncidence();
            if (!incidencePtr) {
                kWarning() << "Failed to read incidence.";
                ok = false;
                return Akonadi::Item();
            }
            Akonadi::Item newItem(incidencePtr->mimeType());
            newItem.setPayload(incidencePtr);
            newItem.setRemoteId(imapItem.remoteId());
            newItem.setGid(incidencePtr->instanceIdentifier());
            return newItem;
        }
        break;
        case Kolab::NoteObject:
        {
            const KMime::Message::Ptr note = reader.getNote();
            if (!note) {
                kWarning() << "Failed to read note.";
                ok = false;
                return Akonadi::Item();
            }
            Akonadi::Item newItem(QLatin1String("text/x-vnd.akonadi.note"));
            newItem.setPayload(note);
            newItem.setRemoteId(imapItem.remoteId());
            const Akonadi::NoteUtils::NoteMessageWrapper wrapper(note);
            newItem.setGid(wrapper.uid());
            return newItem;
        }
        break;
        case Kolab::ContactObject:
        {
            Akonadi::Item newItem(KABC::Addressee::mimeType());
            newItem.setPayload(reader.getContact());
            newItem.setRemoteId(imapItem.remoteId());
            newItem.setGid(reader.getContact().uid());
            return newItem;
        }
        break;
        case Kolab::DistlistObject:
        {
            KABC::ContactGroup contactGroup = reader.getDistlist();

            QList<KABC::ContactGroup::ContactReference> toAdd;
            for (uint index = 0; index < contactGroup.contactReferenceCount(); ++index) {
                const KABC::ContactGroup::ContactReference& reference = contactGroup.contactReference(index);
                KABC::ContactGroup::ContactReference ref;
                ref.setGid(reference.uid()); //libkolab set a gid with setUid()
                toAdd << ref;
            }
            contactGroup.removeAllContactReferences();
            foreach (const KABC::ContactGroup::ContactReference &ref, toAdd) {
                contactGroup.append(ref);
            }

            Akonadi::Item newItem(KABC::ContactGroup::mimeType());
            newItem.setPayload(contactGroup);
            newItem.setRemoteId(imapItem.remoteId());
            newItem.setGid(contactGroup.id());
            return newItem;
        }
        break;
        default:
            kWarning() << "Object type not handled";
            ok = false;
            break;
    }
   return Akonadi::Item();
}

Akonadi::Item::List KolabHelpers::translateToImap(const Akonadi::Item::List &items, bool &ok)
{
    Akonadi::Item::List imapItems;
    Q_FOREACH(const Akonadi::Item &item, items) {
        bool translationOk = true;
        imapItems << translateToImap(item, translationOk);
        if (!translationOk) {
            ok = false;
        }
    }
    return imapItems;
}

static KABC::ContactGroup convertToGidOnly(const KABC::ContactGroup &contactGroup)
{
    QList<KABC::ContactGroup::ContactReference> toAdd;
    for ( uint index = 0; index < contactGroup.contactReferenceCount(); ++index ) {
        const KABC::ContactGroup::ContactReference& reference = contactGroup.contactReference( index );
        QString gid;
        if (!reference.gid().isEmpty()) {
            gid = reference.gid();
        } else {
            // WARNING: this is an ugly hack for backwards compatiblity. Normally this codepath shouldn't be hit.
            // Replace all references with real data-sets
            // Hopefully all resources are available during saving, so we can look up
            // in the addressbook to get name+email from the UID.

            const Akonadi::Item item(reference.uid().toLongLong());
            Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(item);
            job->fetchScope().fetchFullPayload();
            if (!job->exec()) {
                continue;
            }

            const Akonadi::Item::List items = job->items();
            if (items.count() != 1) {
                continue;
            }
            const KABC::Addressee addressee = job->items().first().payload<KABC::Addressee>();
            gid = addressee.uid();
        }
        KABC::ContactGroup::ContactReference ref;
        ref.setUid(gid); //libkolab expects a gid for uid()
        toAdd << ref;
    }
    KABC::ContactGroup gidOnlyContactGroup = contactGroup;
    gidOnlyContactGroup.removeAllContactReferences();
    foreach ( const KABC::ContactGroup::ContactReference &ref, toAdd ) {
      gidOnlyContactGroup.append( ref );
    }
    return gidOnlyContactGroup;
}

Akonadi::Item KolabHelpers::translateToImap(const Akonadi::Item &item, bool &ok)
{
    ok = true;
    //imap messages don't need to be translated
    if (item.mimeType() == KMime::Message::mimeType()) {
        Q_ASSERT(item.hasPayload<KMime::Message::Ptr>());
        return item;
    }
    const QLatin1String productId("Akonadi-Kolab-Resource");
    //Everthing stays the same, except mime type and payload
    Akonadi::Item imapItem = item;
    imapItem.setMimeType( QLatin1String("message/rfc822") );
    switch(getKolabTypeFromMimeType(item.mimeType())) {
        case Kolab::EventObject:
        case Kolab::TodoObject:
        case Kolab::JournalObject:
        {
            Q_ASSERT(item.hasPayload<KCalCore::Incidence::Ptr>());
            kDebug() << "converted event";
            const KMime::Message::Ptr message = Kolab::KolabObjectWriter::writeIncidence(
                item.payload<KCalCore::Incidence::Ptr>(),
                Kolab::KolabV3, productId, QLatin1String("UTC") );
            imapItem.setPayload( message );
        }
        break;
        case Kolab::NoteObject:
        {
            Q_ASSERT(item.hasPayload<KMime::Message::Ptr>());
            kDebug() << "converted note";
            const KMime::Message::Ptr message = Kolab::KolabObjectWriter::writeNote(
                item.payload<KMime::Message::Ptr>(), Kolab::KolabV3, productId);
            imapItem.setPayload( message );
        }
        break;
        case Kolab::ContactObject:
        {
            Q_ASSERT(item.hasPayload<KABC::Addressee>());
            kDebug() << "converted contact";
            const KMime::Message::Ptr message = Kolab::KolabObjectWriter::writeContact(
                item.payload<KABC::Addressee>(), Kolab::KolabV3, productId);
            imapItem.setPayload( message );
        }
        break;
        case Kolab::DistlistObject:
        {
            Q_ASSERT(item.hasPayload<KABC::ContactGroup>());
            const KABC::ContactGroup contactGroup = convertToGidOnly(item.payload<KABC::ContactGroup>());
            kDebug() << "converted distlist";
            const KMime::Message::Ptr message = Kolab::KolabObjectWriter::writeDistlist(
                contactGroup, Kolab::KolabV3, productId);
            imapItem.setPayload( message );
        }
        break;
        default:
            kWarning() << "object type not handled: " << item.id() << item.mimeType();
            ok = false;
            return Akonadi::Item();

    }

    if (checkForErrors(item)) {
        kWarning() << "an error occured while trying to translate the item to the kolab format: " << item.id();
        ok = false;
        return Akonadi::Item();
    }
    return imapItem;
}

QByteArray KolabHelpers::kolabTypeForMimeType( const QStringList &contentMimeTypes )
{
    if (contentMimeTypes.contains(KABC::Addressee::mimeType())) {
        return "contact";
    } else if (contentMimeTypes.contains( KCalCore::Event::eventMimeType())) {
        return "event";
    } else if (contentMimeTypes.contains( KCalCore::Todo::todoMimeType())) {
        return "task";
    } else if (contentMimeTypes.contains( KCalCore::Journal::journalMimeType())) {
        return "journal";
    } else if (contentMimeTypes.contains(QLatin1String("application/x-vnd.akonadi.note")) ||
                contentMimeTypes.contains(QLatin1String("text/x-vnd.akonadi.note"))) {
        return "note";
    }
    return QByteArray();
}

Kolab::ObjectType KolabHelpers::getKolabTypeFromMimeType(const QString &type)
{
    if (type == KCalCore::Event::eventMimeType()) {
        return Kolab::EventObject;
    } else if (type == KCalCore::Todo::todoMimeType()) {
        return Kolab::TodoObject;
    } else if (type == KCalCore::Journal::journalMimeType()) {
        return Kolab::JournalObject;
    } else if (type == KABC::Addressee::mimeType()) {
        return Kolab::ContactObject;
    } else if (type == KABC::ContactGroup::mimeType()) {
        return Kolab::DistlistObject;
    } else if (type == QLatin1String("text/x-vnd.akonadi.note") ||
               type == QLatin1String("application/x-vnd.akonadi.note")) {
        return Kolab::NoteObject;
    }
    return Kolab::InvalidObject;
}

QStringList KolabHelpers::getContentMimeTypes(Kolab::FolderType type)
{
    QStringList contentTypes;
    contentTypes << Akonadi::Collection::mimeType();
    switch (type) {
        case Kolab::EventType:
        case Kolab::TaskType:
        case Kolab::JournalType:
            contentTypes <<  KCalCore::Incidence::mimeTypes();
            break;
        case Kolab::ContactType:
            contentTypes << KABC::Addressee::mimeType() << KABC::ContactGroup::mimeType();
            break;
        case Kolab::NoteType:
            contentTypes << QLatin1String("text/x-vnd.akonadi.note") << QLatin1String("application/x-vnd.akonadi.note");
            break;
        case Kolab::MailType:
            contentTypes << KMime::Message::mimeType();
            break;
        default:
            kDebug() << "unhandled folder type: " << type;
    }
    return contentTypes;
}

Kolab::FolderType KolabHelpers::folderTypeFromString(const QByteArray& folderTypeName)
{
  return Kolab::folderTypeFromString( std::string(folderTypeName.data(), folderTypeName.size()) );
}

QByteArray KolabHelpers::getFolderTypeAnnotation(const QMap< QByteArray, QByteArray > &annotations)
{
    if (annotations.contains("/shared" KOLAB_FOLDER_TYPE_ANNOTATION)) {
        return annotations.value( "/shared" KOLAB_FOLDER_TYPE_ANNOTATION);
    }
    return annotations.value(KOLAB_FOLDER_TYPE_ANNOTATION);
}

void KolabHelpers::setFolderTypeAnnotation(QMap< QByteArray, QByteArray >& annotations, const QByteArray& value)
{
    annotations["/shared" KOLAB_FOLDER_TYPE_ANNOTATION] = value;
}

QString KolabHelpers::getIcon(Kolab::FolderType type)
{
    switch (type) {
        case Kolab::EventType:
        case Kolab::TaskType:
        case Kolab::JournalType:
            return QLatin1String("view-calendar");
        case Kolab::ContactType:
            return QLatin1String("view-pim-contacts");
        case Kolab::NoteType:
            return QLatin1String("view-pim-notes");
        case Kolab::MailType:
        case Kolab::ConfigurationType:
        case Kolab::FreebusyType:
        case Kolab::FileType:
        default:
            break;
    }
    return QString();
}

bool KolabHelpers::isHandledType(Kolab::FolderType type)
{
    switch (type) {
        case Kolab::EventType:
        case Kolab::TaskType:
        case Kolab::JournalType:
        case Kolab::ContactType:
        case Kolab::NoteType:
        case Kolab::MailType:
            return true;
        case Kolab::ConfigurationType:
        case Kolab::FreebusyType:
        case Kolab::FileType:
        default:
            break;
    }
    return false;
}

