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

#include "gmailresource.h"
#include "gmailretrievecollectionstask.h"
#include "gmailretrieveitemstask.h"
#include "gmailresourcestate.h"
#include "gmailpasswordrequester.h"
#include "gmailconfigdialog.h"
#include "gmailsettings.h"
#include "gmailspecialcollectionstask.h"

#include <KLocalizedString>
#include <KWindowSystem>

#include <Akonadi/LinkJob>
#include <Akonadi/CollectionFetchJob>
#include <akonadi/collectionpathresolver_p.h>

#include <imap/sessionpool.h>
#include <imap/sessionuiproxy.h>

#include <KLocale>

GmailResource::GmailResource(const QString &id):
    ImapResourceBase(id)
{
    KGlobal::locale()->insertCatalog("akonadi_imap_resource");
    setSeparatorCharacter('/');

    m_pool->setPasswordRequester(new GmailPasswordRequester(m_pool));
    m_pool->setSessionUiProxy(SessionUiProxy::Ptr(new SessionUiProxy));
}

GmailResource::~GmailResource()
{
}

QString GmailResource::defaultName() const
{
    return i18n("Gmail Resource");
}

KDialog *GmailResource::createConfigureDialog(WId windowId)
{
    GmailConfigDialog *dlg = new GmailConfigDialog(this, windowId);
    KWindowSystem::setMainWindow( dlg, windowId );
    dlg->setWindowIcon( KIcon( QLatin1String("network-server") ) );
    connect(dlg, SIGNAL(finished(int)), this, SLOT(onConfigurationDone(int)));;
    return dlg;
}

void GmailResource::onConfigurationDone(int result)
{
    GmailConfigDialog *dlg = qobject_cast<GmailConfigDialog*>(sender());
    if (result) {
        if ( dlg->shouldClearCache() ) {
            clearCache();
        }
        GmailSettings::self()->writeConfig();
    }
    dlg->deleteLater();
}



ResourceStateInterface::Ptr GmailResource::createResourceState(const TaskArguments &args)
{
    return ResourceStateInterface::Ptr(new GmailResourceState(this, args));
}

void GmailResource::retrieveCollections()
{
    emit status(AgentBase::Running, i18nc("@info:status", "Retrieving folders"));

    ResourceTask *task = new GmailRetrieveCollectionsTask(createResourceState(TaskArguments()), this);
    connect(task, SIGNAL(destroyed(QObject*)),
            this, SLOT(onRetrieveCollectionsDone()));
    task->start(m_pool);
    queueTask(task);
}

void GmailResource::onRetrieveCollectionsDone()
{
    // FIXME: This does not really seem to do what I want it to do...
    GmailSpecialCollectionsTask *task = new GmailSpecialCollectionsTask(createResourceState(TaskArguments()), this);
    task->start(m_pool);
    queueTask(task);;
}


void GmailResource::retrieveItems(const Akonadi::Collection &col)
{
    kDebug() << col.id() << col.remoteId() << col.name();
    if (col.isVirtual()) {
        Akonadi::Collection allMailCol;
        allMailCol.setRemoteId(QLatin1String("/[Gmail]/All Mail"));
        Akonadi::CollectionFetchJob *fetch
            = new Akonadi::CollectionFetchJob(allMailCol, Akonadi::CollectionFetchJob::Base, this);
        connect(fetch, SIGNAL(finished(KJob*)),
                this, SLOT(onRetrieveItemsCollectionRetrieved(KJob*)));
        return;
    }

    setItemStreamingEnabled(true);

    GmailRetrieveItemsTask *task = new GmailRetrieveItemsTask(createResourceState(TaskArguments(col)), this);
    connect(task, SIGNAL(status(int,QString)), SIGNAL(status(int,QString)));
    connect(this, SIGNAL(retrieveNextItemSyncBatch(int)), task, SLOT(onReadyForNextBatch(int)));
    startTask(task);
    scheduleCustomTask(this, "triggerCollectionExtraInfoJobs", QVariant::fromValue(col), ResourceBase::Append);
}

void GmailResource::onRetrieveItemsCollectionRetrieved(KJob *job)
{
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    Akonadi::CollectionFetchJob *fetch = qobject_cast<Akonadi::CollectionFetchJob*>(job);
    if (fetch->collections().count() != 1) {
        kWarning() << "Got" << fetch->collections().count() << "collections, expected only one!";
        cancelTask();
        return;
    }

    retrieveItems(fetch->collections().first());
}


void GmailResource::linkItems(const QByteArray &collectionName, const QStringList &remoteIds)
{
    QString realCollectionName = GmailSettings::self()->accountName();

    if (collectionName == "\\Inbox") {
        realCollectionName += QLatin1String("/INBOX");
    } else if (collectionName == "\\Drafts" || collectionName == "\\Draft") {
        realCollectionName += QLatin1String("/Drafts");
    } else if (collectionName == "\\Important") {
        realCollectionName += QLatin1String("/Important");
    } else if (collectionName == "\\Sent") {
        realCollectionName += QLatin1String("/Sent Mail");
    } else if (collectionName == "\\Junk" || collectionName == "\\Spam") {
        realCollectionName += QLatin1String("/Spam");
    } else if (collectionName == "\\Flagged" || collectionName == "\\Starred") {
        realCollectionName += QLatin1String("/Starred");
    } else if (collectionName == "\\Trash") {
        realCollectionName += QLatin1String("/Trash");
    } else if (collectionName[0] == '/') {
        realCollectionName += QString::fromLatin1(collectionName);
    } else {
        realCollectionName += QLatin1Char('/') + QString::fromLatin1(collectionName);
    }

    kDebug() << "Looking for actual collection for" << realCollectionName << "(originally" << collectionName << ")";
    Akonadi::CollectionPathResolver *resolver
        = new Akonadi::CollectionPathResolver(realCollectionName, this);
    resolver->setProperty("Items", QVariant::fromValue(remoteIds));
    resolver->setProperty("CollectionName", realCollectionName);
    connect(resolver, SIGNAL(finished(KJob*)),
            this, SLOT(onLinkItemsCollectionResolved(KJob*)));
}

void GmailResource::onLinkItemsCollectionResolved(KJob *job)
{
    Akonadi::CollectionPathResolver *resolver
        = qobject_cast<Akonadi::CollectionPathResolver*>(job);
    const QString collectionName = resolver->property("CollectionName").toString();
    if (resolver->error() && resolver->collection() < 0) {
        kWarning() << "Failed to resolve collection ID for path" << collectionName << ":" << resolver->errorString();
        return;
    }
    const Akonadi::Collection collection(resolver->collection());
    const QStringList remoteIds = resolver->property("Items").value<QStringList>();

    kDebug() << "Linking" << remoteIds << "to" << collectionName << "(Collection" << collection.id() << ")";

    Akonadi::Item::List items;
    Q_FOREACH (const QString &remoteId, remoteIds) {
        Akonadi::Item item;
        item.setRemoteId(remoteId);
        items << item;
    }

    Akonadi::LinkJob *linkJob = new Akonadi::LinkJob(collection, items, this);
    linkJob->setProperty("Collection", QVariant::fromValue(collection));
    connect(linkJob, SIGNAL(finished(KJob*)),
            this, SLOT(onLinkingFinished(KJob*)));
}

void GmailResource::onLinkingFinished(KJob *job)
{
    const Akonadi::Collection collection = job->property("Collection").value<Akonadi::Collection>();
    if (job->error()) {
        kWarning() << "Error while linking items to collection" << collection.name() << "(ID" << collection.id() << ")";
        kWarning() << "Error was: " << job->errorString();
    } else {
        kDebug() << "Successfully linked items to" << collection.name() << "(ID" << collection.id() << ")";
    }
}



AKONADI_RESOURCE_MAIN(GmailResource)
