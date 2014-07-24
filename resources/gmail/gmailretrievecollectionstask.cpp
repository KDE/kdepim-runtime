/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
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

#include "gmailretrievecollectionstask.h"
#include "gmaillabelattribute.h"

#include <imap/noselectattribute.h>
#include <imap/noinferiorsattribute.h>

#include <Akonadi/CachePolicy>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/KMime/MessageParts>
#include <Akonadi/EntityHiddenAttribute>

#include <KMime/Message>


#include <KLocalizedString>

GmailRetrieveCollectionsTask::GmailRetrieveCollectionsTask(ResourceStateInterface::Ptr resource,
                                                           QObject *parent)
    : RetrieveCollectionsTask(resource, parent)
{
}

void GmailRetrieveCollectionsTask::onMailBoxesReceived(const QList<KIMAP::MailBoxDescriptor> &descriptors,
                                                       const QList<QList<QByteArray> > &flags)
{
    Akonadi::Collection &rootCollection = m_reportedCollections[QString()];
    Akonadi::EntityDisplayAttribute *attr = rootCollection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Entity::AddIfMissing);
    attr->setIconName(QLatin1String("im-google"));
    attr->setDisplayName(rootCollection.name());

    QStringList contentTypes;
    contentTypes << KMime::Message::mimeType();

    for (int i = 0, dscCnt = descriptors.size(); i < dscCnt; ++i) {
        const KIMAP::MailBoxDescriptor descriptor = descriptors[i];

        // skip phantom mailboxes contained in LSUB but not LIST
        if (isSubscriptionEnabled() && !m_fullReportedCollections.contains(descriptor.name)) {
            kDebug() << "Got phantom mailbox: " << descriptor.name;
            continue;
        }

        const QString boxName = descriptor.name.endsWith(separatorCharacter())
                                ? descriptor.name.left(descriptor.name.size() - 1)
                                : descriptor.name;

        /* FIXME: The Chats folder contains logs of Hangouts chats, which is rather
         * useless in Kmail, and also it breaks sync, because it is not a "special"
         * folder. We should re-enable it at some point so that people can't
         * complain, but until then the folder will not be synced.
         */
         if (boxName == QLatin1String("[Gmail]/Chats")) {
            continue;
        }

        const QStringList pathParts = boxName.split(separatorCharacter());

        QString parentPath;
        QString currentPath;

        for (int j = 0, partsCnt = pathParts.count(); j < partsCnt; ++j ) {
            const bool isDummy = j != pathParts.size() - 1;
            const QString pathPart = pathParts.at(j);
            currentPath += separatorCharacter() + pathPart;

            if (m_reportedCollections.contains(currentPath)) {
                if (m_dummyCollections.contains(currentPath) && !isDummy) {
                    kDebug() << "Received the real collection for a dummy one : " << currentPath;
                    //set the correct attributes for the collection, eg. noselect needs to be removed
                    Akonadi::Collection c = m_reportedCollections.value(currentPath);
                    c.setContentMimeTypes(contentTypes);
                    c.setRights(Akonadi::Collection::AllRights);
                    c.removeAttribute<NoSelectAttribute>();
                    m_dummyCollections.remove(currentPath);
                    m_reportedCollections.insert(currentPath, c);
                }
                parentPath = currentPath;
                continue;
            }

            const QList<QByteArray> currentFlags = isDummy ? (QList<QByteArray>() << "\\noselect") : flags[i];

            Akonadi::Collection c;
            c.setName(pathPart);
            c.setRemoteId(separatorCharacter() + pathPart);
            const Akonadi::Collection parentCollection = m_reportedCollections.value(parentPath);
            c.setParentCollection(parentCollection);
            c.setContentMimeTypes(contentTypes);
            c.setVirtual(true); // All collections are virtual
            c.setRights(Akonadi::Collection::CanChangeCollection |
                        Akonadi::Collection::CanDeleteCollection |
                        Akonadi::Collection::CanCreateCollection |
                        Akonadi::Collection::CanLinkItem |
                        Akonadi::Collection::CanUnlinkItem |
                        Akonadi::Collection::CanCreateItem |
                        Akonadi::Collection::CanDeleteItem |
                        Akonadi::Collection::CanChangeItem);

            Akonadi::EntityDisplayAttribute *attr = c.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Entity::AddIfMissing);
            if (currentFlags.contains("\\trash")) {
                attr->setIconName(QLatin1String("user-trash"));
            } else if (currentFlags.contains("\\sent")) {
                attr->setIconName(QLatin1String("mail-folder-sent"));
            } else if (currentFlags.contains("\\inbox")) {
                attr->setIconName(QLatin1String("mail-folder-inbox"));
            } else {
                attr->setIconName(QLatin1String("folder"));
            }
            attr->setDisplayName(pathPart);

            // If the folder is the Inbox, make some special settings.
            if (currentPath.compare(separatorCharacter() + QLatin1String("INBOX") , Qt::CaseInsensitive) == 0) {
                Akonadi::EntityDisplayAttribute *attr = c.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
                attr->setDisplayName(i18n("Inbox"));
                attr->setIconName(QLatin1String("mail-folder-inbox"));
            }

            // "All Mail", "Trash" and "Spam" are the only non-virtual collections
            // that will store the actual emails.
            if (currentFlags.contains("\\all") || currentFlags.contains("\\trash") || currentFlags.contains("\\junk")) {
                c.setVirtual(false);
                c.addAttribute(new NoSelectAttribute);
                c.addAttribute(new NoInferiorsAttribute);
                c.setParentCollection(m_reportedCollections.value(QString()));
                c.setRemoteId(currentPath);
                c.setLocalListPreference(Akonadi::Collection::ListSync, Akonadi::Collection::ListDefault);

                // Hide "All mail" collection and mark it as IDLE
                if (currentFlags.contains("\\all")) {
                    c.setEnabled(false);
                    c.setLocalListPreference(Akonadi::Collection::ListDisplay, Akonadi::Collection::ListDisabled);
                    c.setLocalListPreference(Akonadi::Collection::ListIndex, Akonadi::Collection::ListDisabled);
                    c.setLocalListPreference(Akonadi::Collection::ListSync, Akonadi::Collection::ListEnabled);

                    // This mean that we will automatically pick up changes in
                    // all labels - YAY!
                    setIdleCollection(c);
                }
                c.setRights(Akonadi::Collection::CanCreateItem |
                            Akonadi::Collection::CanChangeItem |
                            Akonadi::Collection::CanDeleteItem);
            }

            // If this folder is a noselect folder, make some special settings.
            if (currentFlags.contains("\\noselect")) {
                kDebug() << "Dummy collection created: " << currentPath;
                c.addAttribute(new NoSelectAttribute(true));
                c.setContentMimeTypes(QStringList() << Akonadi::Collection::mimeType());
                c.setRights(Akonadi::Collection::ReadOnly);
            } else {
                // remove the noselect attribute explicitly, in case we had set it before (eg. for non-subscribed non-leaf folders)
                c.removeAttribute<NoSelectAttribute>();
            }

            // If this folder is a noinferiors folder, it is not allowed to create subfolders inside.
            if (currentFlags.contains("\\noinferiors")) {
                //kDebug() << "Noinferiors: " << currentPath;
                c.addAttribute(new NoInferiorsAttribute(true));
                c.setRights(c.rights() & ~Akonadi::Collection::CanCreateCollection);
            }

            kDebug() << currentPath << currentFlags;
            // Special treating of Gmail system collections (and INBOX)
            if (currentPath == QLatin1String("/INBOX") ||
                currentFlags.contains("\\drafts") ||
                currentFlags.contains("\\important") ||
                currentFlags.contains("\\sent") ||
                currentFlags.contains("\\flagged"))
            {
                // Keep [Gmail] in remoteID, so that we can reference them correctly
                // even though they have different parent in Akonadi
                c.setRemoteId(currentPath);
                // Move them from the [Gmail] subfolder
                c.setParentCollection(m_reportedCollections.value(QString()));
                // None of these can actually have subcollections, cannot be modified and
                // cannot be removed.
                c.setRights( c.rights() & ~Akonadi::Collection::CanDeleteCollection
                                        & ~Akonadi::Collection::CanChangeCollection
                                        & ~Akonadi::Collection::CanCreateCollection );
            }

            // I am the king of non-generic code!
            if (currentFlags.contains("\\inbox") || pathPart == QLatin1String("INBOX")) {
                c.addAttribute(new GmailLabelAttribute("\\Inbox"));
            } else if (currentFlags.contains("\\drafts")) {
                // This is not a typo, they actually use "\Draft" in X-GM-LABEL
                c.addAttribute(new GmailLabelAttribute("\\Draft"));
            } else if (currentFlags.contains("\\important")) {
                c.addAttribute(new GmailLabelAttribute("\\Important"));
            } else if (currentFlags.contains("\\sent")) {
                c.addAttribute(new GmailLabelAttribute("\\Sent"));
            } else if (currentFlags.contains("\\junk")) {
                c.addAttribute(new GmailLabelAttribute("\\Junk"));
            } else if (currentFlags.contains("\\flagged")) {
                c.addAttribute(new GmailLabelAttribute("\\Flagged"));
            } else if (currentFlags.contains("\\trash")) {
                c.addAttribute(new GmailLabelAttribute("\\Trash"));
            } else if (currentFlags.contains("\\all")) {
                // Ignore
            } else {
                // For non-gmail flags, store the actual path without opening "/",
                // which is actually Gmail label
                c.addAttribute(new GmailLabelAttribute(currentPath.mid(1).toUtf8()));
            }

            // Add special mimetype to non-system virtual collections to allow
            // creating subfolders
            if (c.isVirtual() && c.rights() & Akonadi::Collection::CanCreateCollection) {
                c.setContentMimeTypes(c.contentMimeTypes()
                                      << Akonadi::Collection::virtualMimeType());
            }

            m_reportedCollections.insert(currentPath, c);

            if (isDummy) {
                m_dummyCollections.insert(currentPath, c);
            }

            parentPath = currentPath;
        }
    }

    // Remove the [Gmail] folder. We inserted it only to get remoteIDs for it's subcollections right
    // FIXME GMAIL: Don't hardcode this, try to have some detection or at least a constant
    m_reportedCollections.remove( separatorCharacter() + QLatin1String( "[Gmail]" ) );
}


