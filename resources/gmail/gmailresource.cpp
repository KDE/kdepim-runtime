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
#include "gmaillinkitemstask.h"
#include "gmaillabelattribute.h"
#include "gmailchangeitemslabelstask.h"

#include <KLocalizedString>
#include <KWindowSystem>

#include <AkonadiCore/LinkJob>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/collectionpathresolver.h>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/AttributeFactory>
#include <AkonadiCore/specialcollections.h>

#include <imap/sessionpool.h>
#include <imap/sessionuiproxy.h>

#include <QIcon>

GmailResource::GmailResource(const QString &id)
    : ImapResourceBase(id)
    , m_settings(0)
{
    setSeparatorCharacter(QLatin1Char('/'));

    m_pool->setPasswordRequester(new GmailPasswordRequester(this, m_pool));
    m_pool->setSessionUiProxy(SessionUiProxy::Ptr(new SessionUiProxy));

    Akonadi::AttributeFactory::registerAttribute<GmailLabelAttribute>();
}

GmailResource::~GmailResource()
{
}

Settings *GmailResource::settings() const
{
    if (m_settings == 0) {
        m_settings = new GmailSettings;
    }

    return m_settings;
}

QString GmailResource::defaultName() const
{
    return i18n("Gmail Resource");
}

QDialog *GmailResource::createConfigureDialog(WId windowId)
{
    GmailConfigDialog *dlg = new GmailConfigDialog(this, windowId);
    KWindowSystem::setMainWindow(dlg, windowId);
    dlg->setWindowIcon(QIcon::fromTheme(QLatin1String("network-server")));
    connect(dlg, &QDialog::finished, this, &GmailResource::onConfigurationDone);
    return dlg;
}

void GmailResource::onConfigurationDone(int result)
{
    GmailConfigDialog *dlg = qobject_cast<GmailConfigDialog *>(sender());
    if (result) {
        if (dlg->shouldClearCache()) {
            clearCache();
        }
        settings()->save();
    }
    dlg->deleteLater();
}

Akonadi::Collection GmailResource::allMailCollection() const
{
    Akonadi::Collection c;
    c.setRemoteId(QLatin1String("/[Gmail]/All Mail"));
    return c;
}

Akonadi::Collection GmailResource::rootCollection() const
{
    Akonadi::Collection c;
    c.setRemoteId(settings()->rootRemoteId());
    return c;
}

ResourceStateInterface::Ptr GmailResource::createResourceState(const TaskArguments &args)
{
    return ResourceStateInterface::Ptr(new GmailResourceState(this, args));
}

void GmailResource::retrieveCollections()
{
    emit status(AgentBase::Running, i18nc("@info:status", "Retrieving folders"));

    ResourceTask *task = new GmailRetrieveCollectionsTask(createResourceState(TaskArguments()), this);
    if (settings()->trashCollection() == -1) {
        connect(task, SIGNAL(destroyed(QObject*)),
                this, SLOT(updateTrashFolder()));
    }
    task->start(m_pool);
    queueTask(task);
}

void GmailResource::updateTrashFolder()
{
    Akonadi::CollectionFetchJob *fetch
        = new Akonadi::CollectionFetchJob(rootCollection(), Akonadi::CollectionFetchJob::FirstLevel, this);
    connect(fetch, &KJob::finished,
            this, &GmailResource::onUpdateTrashFolderCollectionsRetrieved);
}

void GmailResource::onUpdateTrashFolderCollectionsRetrieved(KJob *job)
{
    Akonadi::CollectionFetchJob *fetch = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    if (job->error()) {
        kError() << fetch->errorString();
        return;
    }

    Akonadi::Collection::List cols = fetch->collections();
    Q_FOREACH (const Akonadi::Collection &col, cols) {
        GmailLabelAttribute *attr = col.attribute<GmailLabelAttribute>();
        if (!attr) {
            continue;
        }

        if (attr->isTrash()) {
            settings()->setTrashCollection(col.id());
            Akonadi::SpecialCollections::setSpecialCollectionType("trash", col);
            return;
        }
    }

    kWarning() << "Failed to detect the Trash folder...!?";
}

void GmailResource::retrieveItems(const Akonadi::Collection &col)
{
    qDebug() << col.id() << col.remoteId() << col.name();
    // We can't sync the virtual collections - instead we get ID of "All Mail" and
    // we schedule it's sync
    //
    // TODO: Don't resync the All Mail collections X times in a row for each virtual
    // collection, instead uset a timer or something
    if (col.isVirtual()) {
        Akonadi::CollectionFetchJob *fetch
            = new Akonadi::CollectionFetchJob(allMailCollection(), Akonadi::CollectionFetchJob::Base, this);
        connect(fetch, SIGNAL(finished(KJob*)),
                this, SLOT(onRetrieveItemsCollectionRetrieved(KJob*)));
        return;
    }

    setItemStreamingEnabled(true);

    GmailRetrieveItemsTask *task = new GmailRetrieveItemsTask(createResourceState(TaskArguments(col)), this);
    connect(task, SIGNAL(status(int,QString)), SIGNAL(status(int,QString)));
    connect(this, SIGNAL(retrieveNextItemSyncBatch(int)), task, SLOT(onReadyForNextBatch(int)));

    GmailLinkItemsTask *linkTask = new GmailLinkItemsTask(task, this);
    connect(linkTask, SIGNAL(status(int,QString)), SIGNAL(status(int,QString)));

    startTask(task);
    scheduleCustomTask(this, "triggerCollectionExtraInfoJobs", QVariant::fromValue(col), ResourceBase::Append);
}

void GmailResource::onRetrieveItemsCollectionRetrieved(KJob *job)
{
    if (job->error()) {
        cancelTask(job->errorString());
        return;
    }

    Akonadi::CollectionFetchJob *fetch = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    if (fetch->collections().count() != 1) {
        qWarning() << "Got" << fetch->collections().count() << "collections, expected only one!";
        cancelTask();
        return;
    }

    synchronizeCollection(fetch->collections().at(0).id());

    itemsRetrievalDone();
}

void GmailResource::itemsLinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection)
{
    if (!collection.hasAttribute<GmailLabelAttribute>()) {
        qWarning() << "Collection is missing GmailLabelAttribute! IMPOSSIBRU!";
        cancelTask();
        return;
    }

    const QByteArray label = collection.attribute<GmailLabelAttribute>()->label();
    TaskArguments args(items, QSet<QByteArray>() << label, QSet<QByteArray>());
    GmailChangeItemsLabelsTask *task = new GmailChangeItemsLabelsTask(createResourceState(args), this);
    startTask(task);
}

void GmailResource::itemsUnlinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection)
{
    if (!collection.hasAttribute<GmailLabelAttribute>()) {
        qWarning() << "Collection is missing GmailLabelAttribute! IMPOSSIBRU!";
        cancelTask();
        return;
    }

    const QByteArray label = collection.attribute<GmailLabelAttribute>()->label();
    TaskArguments args(items, QSet<QByteArray>(), QSet<QByteArray>() << label);
    GmailChangeItemsLabelsTask *task = new GmailChangeItemsLabelsTask(createResourceState(args), this);
    startTask(task);
}

AKONADI_RESOURCE_MAIN(GmailResource)
