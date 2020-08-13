/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "akonadislave.h"

#include <itemfetchjob.h>
#include <itemfetchscope.h>
#include <itemdeletejob.h>
#include <collection.h>
#include <collectionfetchjob.h>
#include <collectiondeletejob.h>
#include <entitydisplayattribute.h>

#include "akonadislave_debug.h"

#include <QApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QCommandLineOption>

extern "C" {
int Q_DECL_EXPORT kdemain(int argc, char **argv);
}

int kdemain(int argc, char **argv)
{
    QApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("kio_akonadi"), QString(), QStringLiteral("0"));
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addOption(QCommandLineOption(QStringList() <<  QStringLiteral("+protocol"), i18n("Protocol name")));
    parser.addOption(QCommandLineOption(QStringList() <<  QStringLiteral("+pool"), i18n("Socket name")));
    parser.addOption(QCommandLineOption(QStringList() <<  QStringLiteral("+app"), i18n("Socket name")));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    AkonadiSlave slave(parser.positionalArguments().at(1).toLocal8Bit(), parser.positionalArguments().at(2).toLocal8Bit());
    slave.dispatchLoop();

    return 0;
}

using namespace Akonadi;

AkonadiSlave::AkonadiSlave(const QByteArray &pool_socket, const QByteArray &app_socket)
    : KIO::SlaveBase("akonadi", pool_socket, app_socket)
{
    qCDebug(AKONADISLAVE_LOG) << "kio_akonadi starting up";
}

AkonadiSlave::~AkonadiSlave()
{
    qCDebug(AKONADISLAVE_LOG) << "kio_akonadi shutting down";
}

void AkonadiSlave::get(const QUrl &url)
{
    const Item item = Item::fromUrl(url);
    ItemFetchJob *job = new ItemFetchJob(item);
    job->fetchScope().fetchFullPayload();

    if (!job->exec()) {
        error(KIO::ERR_INTERNAL, job->errorString());
        return;
    }

    if (job->items().count() != 1) {
        error(KIO::ERR_DOES_NOT_EXIST, i18n("No such item."));
    } else {
        const Item item = job->items().at(0);
        QByteArray tmp = item.payloadData();
        data(tmp);
        data(QByteArray());
        finished();
    }

    finished();
}

void AkonadiSlave::stat(const QUrl &url)
{
    qCDebug(AKONADISLAVE_LOG) << url;

    // Stats for a collection
    if (Collection::fromUrl(url).isValid()) {
        Collection collection = Collection::fromUrl(url);

        if (collection != Collection::root()) {
            // Check that the collection exists.
            CollectionFetchJob *job = new CollectionFetchJob(collection, CollectionFetchJob::Base);
            if (!job->exec()) {
                error(KIO::ERR_INTERNAL, job->errorString());
                return;
            }

            if (job->collections().count() != 1) {
                error(KIO::ERR_DOES_NOT_EXIST, i18n("No such item."));
                return;
            }

            collection = job->collections().at(0);
        }

        statEntry(entryForCollection(collection));
        finished();
    }
    // Stats for an item
    else if (Item::fromUrl(url).isValid()) {
        ItemFetchJob *job = new ItemFetchJob(Item::fromUrl(url));

        if (!job->exec()) {
            error(KIO::ERR_INTERNAL, job->errorString());
            return;
        }

        if (job->items().count() != 1) {
            error(KIO::ERR_DOES_NOT_EXIST, i18n("No such item."));
            return;
        }

        const Item item = job->items().at(0);
        statEntry(entryForItem(item));
        finished();
    }
}

void AkonadiSlave::del(const QUrl &url, bool isFile)
{
    qCDebug(AKONADISLAVE_LOG) << url;

    if (!isFile) {                   // It's a directory
        Collection collection = Collection::fromUrl(url);
        CollectionDeleteJob *job = new CollectionDeleteJob(collection);
        if (!job->exec()) {
            error(KIO::ERR_INTERNAL, job->errorString());
            return;
        }
        finished();
    } else {                         // It's a file
        ItemDeleteJob *job = new ItemDeleteJob(Item::fromUrl(url));
        if (!job->exec()) {
            error(KIO::ERR_INTERNAL, job->errorString());
            return;
        }
        finished();
    }
}

void AkonadiSlave::listDir(const QUrl &url)
{
    qCDebug(AKONADISLAVE_LOG) << url;

    if (!Collection::fromUrl(url).isValid()) {
        error(KIO::ERR_DOES_NOT_EXIST, i18n("No such collection."));
        return;
    }

    // Fetching collections
    Collection collection = Collection::fromUrl(url);
    if (!collection.isValid()) {
        error(KIO::ERR_DOES_NOT_EXIST, i18n("No such collection."));
        return;
    }
    CollectionFetchJob *job = new CollectionFetchJob(collection, CollectionFetchJob::FirstLevel);
    if (!job->exec()) {
        error(KIO::ERR_CANNOT_ENTER_DIRECTORY, job->errorString());
        return;
    }

    const Collection::List collections = job->collections();
    for (const Collection &col : collections) {
        listEntry(entryForCollection(col));
    }

    // Fetching items
    if (collection != Collection::root()) {
        ItemFetchJob *fjob = new ItemFetchJob(collection);
        if (!fjob->exec()) {
            error(KIO::ERR_INTERNAL, job->errorString());
            return;
        }
        const Item::List items = fjob->items();
        totalSize(collections.count() + items.count());
        for (const Item &item : items) {
            listEntry(entryForItem(item));
        }
    }

    finished();
}

KIO::UDSEntry AkonadiSlave::entryForItem(const Akonadi::Item &item)
{
    KIO::UDSEntry entry;
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QString::number(item.id()));
    entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, item.mimeType());
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    entry.fastInsert(KIO::UDSEntry::UDS_URL, item.url().url());
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, item.size());
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH);
    entry.fastInsert(KIO::UDSEntry::UDS_MODIFICATION_TIME, item.modificationTime().toSecsSinceEpoch());
    return entry;
}

KIO::UDSEntry AkonadiSlave::entryForCollection(const Akonadi::Collection &collection)
{
    KIO::UDSEntry entry;
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, collection.name());
    entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, Collection::mimeType());
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_URL, collection.url().url());
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH);
    if (const EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>()) {
        if (!attr->iconName().isEmpty()) {
            entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, attr->iconName());
        }
        if (!attr->displayName().isEmpty()) {
            entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, attr->displayName());
        }
    }
    return entry;
}
