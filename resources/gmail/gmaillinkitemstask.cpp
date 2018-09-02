/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "gmaillinkitemstask.h"
#include "gmailresource.h"
#include "gmailretrieveitemstask.h"
#include "gmailsettings.h"

#include <AkonadiCore/collectionpathresolver.h>
#include <AkonadiAgentBase/AgentBase>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/LinkJob>
#include <AkonadiCore/UnlinkJob>

#include <KLocalizedString>

#define LABEL_PROPERTY "LabelProperty"
#define COLLECTION_NAME_PROPERTY "CollectionNameProperty"
#define COLLECTION_PROPERTY "CollectionProperty"

GmailLinkItemsTask::GmailLinkItemsTask(GmailRetrieveItemsTask *retrieveTask, GmailResource *parent)
    : QObject(parent)
    , mResource(parent)
{
    connect(retrieveTask, SIGNAL(linkItem(QString,QVector<QByteArray>)),
            this, SLOT(linkItem(QString,QVector<QByteArray>)));
    connect(retrieveTask, SIGNAL(destroyed(QObject*)),
            this, SLOT(onRetrievalDone()));
}

GmailLinkItemsTask::~GmailLinkItemsTask()
{
}

void GmailLinkItemsTask::emitDone()
{
    Q_EMIT done();
    Q_EMIT status(Akonadi::AgentBase::Idle);
    deleteLater();
}

void GmailLinkItemsTask::linkItem(const QString &remoteId, const QVector<QByteArray> &labels)
{
    mLinkMap.insert(remoteId, labels);
    Q_FOREACH (const QByteArray &label, labels) {
        if (!mLabels.contains(label)) {
            mLabels << label;
        }
    }
}

void GmailLinkItemsTask::onRetrievalDone()
{
    Q_EMIT status(Akonadi::AgentBase::Running, i18n("Linking emails to labels"));
    if (!mLabels.isEmpty()) {
        resolveNextLabel();
    } else {
        emitDone();
    }
}

// Step 1: Resolve all labels to collections
void GmailLinkItemsTask::resolveNextLabel()
{
    const QByteArray label = mLabels.takeFirst();

    QString realCollectionName;
    if (label == "\\Inbox") {
        realCollectionName = QLatin1String("/INBOX");
    } else if (label == "\\Drafts" || label == "\\Draft") {
        realCollectionName = QLatin1String("/Drafts");
    } else if (label == "\\Important") {
        realCollectionName = QLatin1String("/Important");
    } else if (label == "\\Sent") {
        realCollectionName = QLatin1String("/Sent Mail");
    } else if (label == "\\Junk" || label == "\\Spam") {
        realCollectionName = QLatin1String("/Spam");
    } else if (label == "\\Flagged" || label == "\\Starred") {
        realCollectionName = QLatin1String("/Starred");
    } else if (label == "\\Trash") {
        realCollectionName = QLatin1String("/Trash");
    } else if (label[0] == '/') {
        realCollectionName = QString::fromLatin1(label);
    } else {
        realCollectionName = QLatin1Char('/') + QString::fromLatin1(label);
    }

    Akonadi::Collection rootCollection;
    rootCollection.setRemoteId(mResource->settings()->rootRemoteId());
    Akonadi::CollectionPathResolver *resolver
        = new Akonadi::CollectionPathResolver(realCollectionName, rootCollection, this);
    resolver->setProperty(COLLECTION_NAME_PROPERTY, realCollectionName);
    resolver->setProperty(LABEL_PROPERTY, label);
    connect(resolver, SIGNAL(finished(KJob*)),
            this, SLOT(onLabelResolved(KJob*)));
}

// Step 2: Continue resolving until all is resolved, then go to step 3
void GmailLinkItemsTask::onLabelResolved(KJob *job)
{
    Akonadi::CollectionPathResolver *resolver
        = qobject_cast<Akonadi::CollectionPathResolver *>(job);
    const QString collectionName = resolver->property(COLLECTION_NAME_PROPERTY).toString();
    const QByteArray label = resolver->property(LABEL_PROPERTY).toByteArray();
    if (resolver->error() && resolver->collection() < 0) {
        qWarning() << "Failed to resolve collection ID for path" << collectionName << ":" << resolver->errorString();
        return;
    }
    const Akonadi::Collection collection(resolver->collection());
    mLabelCollectionMap.insert(label, collection);

    if (!mLabels.isEmpty()) {
        resolveNextLabel();
    } else {
        retrieveVirtualReferences();
    }
}

// Step 3: Retrieve virtual references ot all our items
void GmailLinkItemsTask::retrieveVirtualReferences()
{
    Akonadi::Collection allMailCollection;
    allMailCollection.setRemoteId(QLatin1String("/[Gmail]/All Mail"));

    Akonadi::Item::List items;
    Q_FOREACH (const QString &remoteId, mLinkMap.uniqueKeys()) {
        Akonadi::Item item;
        item.setRemoteId(remoteId);
        items << item;
    }
    Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(items, this);
    fetchJob->setCollection(allMailCollection);
    fetchJob->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::None);
    fetchJob->fetchScope().setCacheOnly(true);
    fetchJob->fetchScope().setFetchModificationTime(false);
    fetchJob->fetchScope().setFetchRemoteIdentification(true);
    fetchJob->fetchScope().setFetchVirtualReferences(true);
    connect(fetchJob, SIGNAL(finished(KJob*)),
            this, SLOT(onVirtualReferencesRetrieved(KJob*)));
}

// Step 4: Compare existing virtual references and our current labels, link and
// unlink as necessary
void GmailLinkItemsTask::onVirtualReferencesRetrieved(KJob *job)
{
    if (job->error()) {
        // TODO: Error handling
        emitDone();
        return;
    }

    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);

    QMap<Akonadi::Collection, Akonadi::Item::List> toLink;
    QMap<Akonadi::Collection, Akonadi::Item::List> toUnlink;
    Q_FOREACH (const Akonadi::Item &item, fetchJob->items()) {
        Akonadi::Collection::List existingReferences = item.virtualReferences();
        const QVector<QByteArray> newLabels = mLinkMap[item.remoteId()];
        Akonadi::Collection::List newReferences;
        Q_FOREACH (const QByteArray &label, newLabels) {
            const Akonadi::Collection newRef = mLabelCollectionMap[label];
            if (!existingReferences.contains(newRef)) {
                Akonadi::Item::List &list = toLink[newRef];
                list << item;
            } else {
                existingReferences.removeOne(newRef);
            }
        }
        if (!existingReferences.isEmpty()) {
            Q_FOREACH (const Akonadi::Collection &ref, existingReferences) {
                Akonadi::Item::List &list = toUnlink[ref];
                list << item;
            }
        }
    }

    QMap<Akonadi::Collection, Akonadi::Item::List>::ConstIterator iter;
    for (iter = toLink.constBegin(); iter != toLink.constEnd(); ++iter) {
        new Akonadi::LinkJob(iter.key(), iter.value());
    }
    for (iter = toUnlink.constBegin(); iter != toUnlink.constEnd(); ++iter) {
        new Akonadi::UnlinkJob(iter.key(), iter.value());
    }

    emitDone();
}
