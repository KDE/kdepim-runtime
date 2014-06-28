/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>
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

#include "kolabretrievecollectionstask.h"
#include "kolabhelpers.h"

#include <noselectattribute.h>
#include <noinferiorsattribute.h>
#include <collectionannotationsattribute.h>

#include <akonadi/cachepolicy.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/kmime/messageparts.h>

#include <kmime/kmime_message.h>

#include <KDE/KDebug>
#include <KDE/KLocale>

KolabRetrieveCollectionsTask::KolabRetrieveCollectionsTask(ResourceStateInterface::Ptr resource, QObject* parent)
    : ResourceTask(CancelIfNoSession, resource, parent),
    mJobs(0)
{

}

KolabRetrieveCollectionsTask::~KolabRetrieveCollectionsTask()
{

}

void KolabRetrieveCollectionsTask::doStart(KIMAP::Session *session)
{
    Akonadi::Collection root;
    root.setName(resourceName());
    root.setRemoteId(rootRemoteId());
    root.setContentMimeTypes(QStringList(Akonadi::Collection::mimeType()));
    root.setParentCollection(Akonadi::Collection::root());
    root.addAttribute(new NoSelectAttribute(true));
    root.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing)->setIconName(QLatin1String("kolab"));

    Akonadi::CachePolicy policy;
    policy.setInheritFromParent(false);
    policy.setSyncOnDemand(true);

    QStringList localParts;
    localParts << QLatin1String(Akonadi::MessagePart::Envelope)
                << QLatin1String(Akonadi::MessagePart::Header);
    int cacheTimeout = 60;

    if (isDisconnectedModeEnabled()) {
        // For disconnected mode we also cache the body
        // and we keep all data indifinitely
        localParts << QLatin1String(Akonadi::MessagePart::Body);
        cacheTimeout = -1;
    }

    policy.setLocalParts(localParts);
    policy.setCacheTimeout(cacheTimeout);
    policy.setIntervalCheckTime(intervalCheckTime());

    root.setCachePolicy(policy);

    mMailCollections.insert(QString(), root);

    //jobs are serialized by the session
    if (isSubscriptionEnabled()) {
        KIMAP::ListJob *fullListJob = new KIMAP::ListJob(session);
        fullListJob->setOption(KIMAP::ListJob::NoOption);
        fullListJob->setQueriedNamespaces(serverNamespaces());
        connect( fullListJob, SIGNAL(mailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)),
                this, SLOT(onFullMailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)) );
        connect( fullListJob, SIGNAL(result(KJob*)), SLOT(onFullMailBoxesReceiveDone(KJob*)));
        mJobs++;
        fullListJob->start();
    }

    KIMAP::ListJob *listJob = new KIMAP::ListJob(session);
    listJob->setOption(KIMAP::ListJob::IncludeUnsubscribed);
    listJob->setQueriedNamespaces(serverNamespaces());
    connect(listJob, SIGNAL(mailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)),
            this, SLOT(onMailBoxesReceived(QList<KIMAP::MailBoxDescriptor>,QList<QList<QByteArray> >)));
    connect(listJob, SIGNAL(result(KJob*)), SLOT(onMailBoxesReceiveDone(KJob*)));
    mJobs++;
    listJob->start();
}

void KolabRetrieveCollectionsTask::onMailBoxesReceived(const QList< KIMAP::MailBoxDescriptor > &descriptors,
                                                   const QList< QList<QByteArray> > &flags)
{
    for (int i=0; i<descriptors.size(); ++i) {
        const KIMAP::MailBoxDescriptor descriptor = descriptors[i];

        //TODO Get all folders anyways, but locally unsubscribe from the ones we're not subscribed to online?
        if (isSubscriptionEnabled() && !mSubscribedMailboxes.contains(descriptor.name)) {
            // not subscribed, skipping
            continue;
        }

        createCollection(descriptor.name, flags.at(i));
    }
    checkDone();
}

Akonadi::Collection KolabRetrieveCollectionsTask::getOrCreateParent(const QString &path)
{
    if (mMailCollections.contains(path)) {
        return mMailCollections.value(path);
    }
    //create a dummy collection
    const QString separator = separatorCharacter();
    const QStringList pathParts = path.split(separator);
    const QString pathPart = pathParts.last();
    Akonadi::Collection c;
    c.setName( pathPart );
    c.setRemoteId( separator + pathPart );
    const QStringList parentPath = pathParts.mid(0, pathParts.size() - 1);
    const Akonadi::Collection parentCollection = getOrCreateParent(parentPath.join(separator));
    c.setParentCollection(parentCollection);

    c.addAttribute(new NoSelectAttribute(true));
    c.setContentMimeTypes(QStringList() << Akonadi::Collection::mimeType());
    c.setRights(Akonadi::Collection::ReadOnly);
    mMailCollections.insert(path, c);
    return c;
}

void KolabRetrieveCollectionsTask::createCollection(const QString &mailbox, const QList<QByteArray> &currentFlags)
{
    const QString separator = separatorCharacter();
    Q_ASSERT(separator.size() == 1);
    const QString boxName = mailbox.endsWith( separator )
                          ? mailbox.left( mailbox.size()-1 )
                          : mailbox;
    const QStringList pathParts = boxName.split( separator );
    const QString pathPart = pathParts.last();

    Akonadi::Collection c;
    //If we had a dummy collection we need to replace it
    if (mMailCollections.contains(mailbox)) {
        c = mMailCollections.value(mailbox);
    }
    c.setName( pathPart );
    c.setRemoteId( separator + pathPart );
    const QStringList parentPath = pathParts.mid(0, pathParts.size() - 1);
    const Akonadi::Collection parentCollection = getOrCreateParent(parentPath.join(separator));
    c.setParentCollection(parentCollection);
    //TODO get from ResourceState, and add KMime::Message::mimeType() for the normal imap resource by default
    //We add a dummy mimetype, otherwise the itemsync doesn't even work (action is disabled and resourcebase aborts the operation)
    c.setContentMimeTypes(QStringList() << Akonadi::Collection::mimeType() << QLatin1String("application/x-kolab-objects"));
    //assume LRS, until myrights is executed
    if (serverCapabilities().contains(QLatin1String("ACL"))) {
        c.setRights(Akonadi::Collection::ReadOnly);
    } else {
        c.setRights(Akonadi::Collection::AllRights);
    }

    // If the folder is the Inbox, make some special settings.
    if (pathParts.size() == 1 && pathPart.compare(QLatin1String("inbox") , Qt::CaseInsensitive) == 0) {
        Akonadi::EntityDisplayAttribute *attr = c.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        attr->setDisplayName(i18n("Inbox"));
        attr->setIconName(QLatin1String("mail-folder-inbox"));
        setIdleCollection(c);
    }

    // If the folder is the user top-level folder, mark it as well, although it is not officially noted in the RFC
    if (pathParts.size() == 1 && pathPart == QLatin1String("user") && currentFlags.contains("\\noselect")) {
        Akonadi::EntityDisplayAttribute *attr = c.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        attr->setDisplayName(i18n("Shared Folders"));
        attr->setIconName(QLatin1String("x-mail-distribution-list"));
    }

    // If this folder is a noselect folder, make some special settings.
    if (currentFlags.contains("\\noselect")) {
        c.addAttribute(new NoSelectAttribute(true));
        c.setContentMimeTypes(QStringList() << Akonadi::Collection::mimeType());
        c.setRights( Akonadi::Collection::ReadOnly );
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

    kDebug() << "creating collection " << mailbox << " with parent " << parentPath;
    mMailCollections.insert(mailbox, c);
    //This is no longer required
    mSubscribedMailboxes.remove(mailbox);
}

void KolabRetrieveCollectionsTask::onMailBoxesReceiveDone(KJob* job)
{
    mJobs--;
    if (job->error()) {
        cancelTask(job->errorString());
    } else {
        checkDone();
    }
}

void KolabRetrieveCollectionsTask::checkDone()
{
    if (!mJobs) {
        collectionsRetrieved(mMailCollections.values());
    }
}

void KolabRetrieveCollectionsTask::onFullMailBoxesReceived(const QList< KIMAP::MailBoxDescriptor >& descriptors,
                                                       const QList< QList< QByteArray > >& flags)
{
    Q_UNUSED(flags);
    foreach (const KIMAP::MailBoxDescriptor &descriptor, descriptors) {
        mSubscribedMailboxes.insert(descriptor.name);
    }
}

void KolabRetrieveCollectionsTask::onFullMailBoxesReceiveDone(KJob* job)
{
    mJobs--;
    if (job->error()) {
        cancelTask(job->errorString());
    } else {
        checkDone();
    }
}
