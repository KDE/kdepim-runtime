/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphfetchfoldersjob.h"

#include "graphclient/graphrequest.h"

#include <Akonadi/Collection>
#include <KLocalizedString>
#include <KMime/Message>
#include <QJsonArray>
#include <QJsonObject>

using namespace Akonadi;

GraphFetchFoldersJob::GraphFetchFoldersJob(GraphClient &client, const Collection &root, const QString &deltaLink, QObject *parent)
    : KJob(parent)
    , mClient(client)
    , mRoot(root)
    , mDeltaLink(deltaLink)
    , mIncremental(!deltaLink.isEmpty())
{
}

void GraphFetchFoldersJob::start()
{
    // Folders reference their parent by the *real* id of the mailbox root, not by the
    // "msgfolderroot" well-known alias — resolve it first so the tree links up.
    auto req = new GraphRequest(mClient, this);
    req->setPath(QStringLiteral("/me/mailFolders/msgfolderroot?$select=id"));
    connect(req, &KJob::result, this, &GraphFetchFoldersJob::onRootResolved);
    req->start();
}

void GraphFetchFoldersJob::onRootResolved(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }
    auto *req = qobject_cast<GraphRequest *>(job);
    const QString rootId = req->responseObject().value(QLatin1String("id")).toString();
    if (rootId.isEmpty()) {
        setError(KJob::UserDefinedError);
        setErrorText(i18n("Could not resolve the mailbox root folder"));
        emitResult();
        return;
    }
    mRoot.setRemoteId(rootId);
    startDelta();
}

void GraphFetchFoldersJob::startDelta()
{
    auto req = new GraphRequest(mClient, this);
    if (mIncremental) {
        req->setAbsoluteUrl(QUrl(mDeltaLink)); // stored @odata.deltaLink
    } else {
        // Select only the fields we map; request delta so we get a deltaLink back.
        req->setPath(QStringLiteral("/me/mailFolders/delta?$select=id,displayName,parentFolderId,totalItemCount"));
    }
    connect(req, &KJob::result, this, &GraphFetchFoldersJob::onRequestFinished);
    req->start();
}

Collection GraphFetchFoldersJob::collectionFromJson(const QJsonObject &folder) const
{
    Collection col;
    col.setRemoteId(folder.value(QLatin1String("id")).toString());
    col.setName(folder.value(QLatin1String("displayName")).toString());
    // Mail folders; Collection::mimeType() so subfolders can live underneath.
    col.setContentMimeTypes({Collection::mimeType(), KMime::Message::mimeType()});

    const QString parentId = folder.value(QLatin1String("parentFolderId")).toString();
    Collection parent;
    parent.setRemoteId(parentId.isEmpty() ? mRoot.remoteId() : parentId);
    col.setParentCollection(parent);

    col.setRights(Collection::AllRights);
    return col;
}

void GraphFetchFoldersJob::onRequestFinished(KJob *job)
{
    auto *req = qobject_cast<GraphRequest *>(job);
    if (job->error()) {
        if (req->httpStatus() == 410 && mIncremental) {
            // The stored delta token expired (HTTP 410 Gone) — fall back to a full
            // folder tree sync, which also produces a fresh deltaLink.
            mDeltaLink.clear();
            mIncremental = false;
            mAll.clear();
            mChanged.clear();
            mRemoved.clear();
            startDelta();
            return;
        }
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }
    mDeltaLink = req->deltaLink();

    const QJsonArray folders = req->aggregatedValue();
    for (const auto &v : folders) {
        const QJsonObject folder = v.toObject();
        if (folder.contains(QLatin1String("@removed"))) {
            Collection col;
            col.setRemoteId(folder.value(QLatin1String("id")).toString());
            mRemoved.append(col);
        } else {
            const Collection col = collectionFromJson(folder);
            mChanged.append(col);
            mAll.append(col);
        }
    }
    emitResult();
}

bool GraphFetchFoldersJob::isIncremental() const
{
    return mIncremental;
}

Collection::List GraphFetchFoldersJob::allCollections() const
{
    return mAll;
}

Collection::List GraphFetchFoldersJob::changedCollections() const
{
    return mChanged;
}

Collection::List GraphFetchFoldersJob::removedCollections() const
{
    return mRemoved;
}

QString GraphFetchFoldersJob::deltaLink() const
{
    return mDeltaLink;
}

Collection GraphFetchFoldersJob::rootCollection() const
{
    return mRoot;
}

#include "moc_graphfetchfoldersjob.cpp"
