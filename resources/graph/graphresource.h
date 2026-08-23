/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>

    SPDX-License-Identifier: LGPL-2.0-or-later

    Akonadi resource for Microsoft 365 / Exchange Online using the Microsoft Graph API.
    Structure mirrors resources/ews (EWS resource) so it can be upstreamed easily.
*/

#pragma once

#include <QScopedPointer>

#include <Akonadi/ResourceWidgetBase>
#include <Akonadi/TransportResourceBase>

#include "graphclient/graphclient.h"

#include <QJsonObject>

#include <functional>
#include <memory>

class GraphSettings;
class GraphOAuth;
class QTimer;

/**
 * Receiving resource: folders + messages (+ later calendar/contacts).
 *
 * ResourceBase drives synchronisation by calling the retrieve*() slots below.
 * Each retrieve*() kicks off an async KJob (see jobs/), and on completion we hand the
 * result back to Akonadi via collectionsRetrieved()/itemsRetrieved()/changeCommitted().
 *
 * The AgentBase::ObserverV3 overrides implement *change replay*: local edits in KMail
 * are pushed back to Graph (PATCH/POST/DELETE ...).
 *
 * Sending: the separate MTA agent (graphmtaresource) forwards the MIME content here
 * over D-Bus (sendMessage/messageSent), so only one process holds the OAuth tokens.
 */
class GraphResource : public Akonadi::ResourceWidgetBase, public Akonadi::AgentBase::ObserverV3, public Akonadi::TransportResourceBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Graph.Resource")
public:
    explicit GraphResource(const QString &id);
    ~GraphResource() override;

    [[nodiscard]] GraphSettings *settings()
    {
        return mSettings.data();
    }

    [[nodiscard]] const Akonadi::Collection &rootCollection() const
    {
        return mRootCollection;
    }

    // --- Change replay (local -> Graph) --------------------------------------
    // Collections == Graph mailFolders
    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override; // POST /me/mailFolders
    void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes) override; // PATCH /me/mailFolders/{id}
    void collectionRemoved(const Akonadi::Collection &collection) override; // DELETE /me/mailFolders/{id}
    void collectionMoved(const Akonadi::Collection &collection,
                         const Akonadi::Collection &source,
                         const Akonadi::Collection &destination) override; // POST /me/mailFolders/{id}/move

    // Items == Graph messages
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override; // POST /me/mailFolders/{id}/messages
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers) override; // PATCH /me/messages/{id}
    void itemsFlagsChanged(const Akonadi::Item::List &items,
                           const QSet<QByteArray> &addedFlags,
                           const QSet<QByteArray> &removedFlags) override; // PATCH isRead / flag
    void itemsMoved(const Akonadi::Item::List &items,
                    const Akonadi::Collection &sourceCollection,
                    const Akonadi::Collection &destinationCollection) override; // POST /me/messages/{id}/move
    void itemsRemoved(const Akonadi::Item::List &items) override; // DELETE /me/messages/{id}

    // Sending is delegated to the separate MTA resource; kept here for on-prem/testing.
    void sendItem(const Akonadi::Item &item) override;

    /// D-Bus entry point for the MTA agent: create a draft from MIME and send it.
    Q_SCRIPTABLE void sendMessage(const QString &id, const QByteArray &content);

    /// Drop the persisted folder deltaLink so the next sync is a full one (recovery).
    Q_SCRIPTABLE void clearFolderSyncState();

Q_SIGNALS:
    /// D-Bus reply to sendMessage: error is empty on success.
    Q_SCRIPTABLE void messageSent(const QString &id, const QString &error);

protected:
    void doSetOnline(bool online) override;

protected Q_SLOTS:
    // --- Synchronisation (Graph -> local) ------------------------------------
    /// Full/delta folder list: GET /me/mailFolders/delta -> collectionsRetrieved[Incremental]().
    void retrieveCollections() override;
    /// Per-folder message delta: GET /me/mailFolders/{id}/messages/delta -> itemsRetrieved[Incremental]().
    void retrieveItems(const Akonadi::Collection &collection) override;
    /// On-demand payloads: GET /me/messages/{id}/$value -> KMime, then itemsRetrieved().
    bool retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;

private:
    void delayedInit();
    // Config was changed from outside (graphconfig plugin in the client process).
    void reloadConfig();
    // Keep the account root collection's name in sync with instance renames.
    void updateRootCollectionName(const QString &name);
    void onAuthReady();
    void onAuthFailed(const QString &error);

    void fetchFoldersJobFinished(KJob *job);
    void fetchItemsJobFinished(KJob *job);
    void fetchPimItemsJobFinished(KJob *job);
    void fetchPayloadJobFinished(KJob *job);

    void setUpAuth();
    void reconfigureClient();
    // Entered once the one-time remoteId migration has run (see onAuthReady).
    void startSyncing();
    // Incremental delivery with tombstones resolved against the local cache and the
    // deltaLink persisted only after the ItemSync committed (both via own ItemSync —
    // itemsRetrievedIncremental() cannot sequence the save after the commit).
    void deliverItemsIncremental(const Akonadi::Collection &collection,
                                 const Akonadi::Item::List &changed,
                                 const Akonadi::Item::List &removed,
                                 const QString &deltaLink);
    void syncItemsIncremental(const Akonadi::Collection &collection,
                              const Akonadi::Item::List &changed,
                              const Akonadi::Item::List &removed,
                              const QString &deltaLink);
    // Deliver on-demand payloads on freshly fetched items so the store cannot race
    // a concurrent delta (revision conflict) or clobber newer flags.
    void deliverFreshPayloads(const Akonadi::Item::List &filled);

    void fetchFolderTree();
    // Resolve removed-collection tombstones against the local cache before delivery —
    // CollectionSync drops removed entries whose parent is unknown.
    void deliverIncrementalTree(const Akonadi::Collection::List &changed, const Akonadi::Collection::List &removed);
    void fetchExtraCollections(); // calendars + contacts + todo lists
    // Continues fetchExtraCollections(); mExtraCollections is only replaced once both
    // list requests have succeeded, so a transient failure cannot surface as deletions.
    void fetchTodoListCollections(Akonadi::Collection::List fresh);
    // Tag Inbox/Sent/Drafts/Trash/Junk/Outbox as Akonadi special collections so KMail
    // and the unified-mailbox agent treat them correctly. Applied inline during sync.
    void applySpecialAttributes(Akonadi::Collection &col);
    // Adopt Graph's server-side sent copy instead of creating a duplicate.
    void reconcileSentItem(const Akonadi::Item &item, const QString &messageId);
    // Draft edits: Graph cannot replace MIME in place — recreate the draft, drop the old one.
    [[nodiscard]] bool isDraftsCollection(const Akonadi::Collection &col) const;
    void replaceDraft(const Akonadi::Item &item);
    // MIME ingestion; calls done(remoteId, errorText) with an empty id on failure.
    void createMimeMessage(const QByteArray &rawMime,
                           const QByteArray &contentType,
                           const QString &targetFolderRid,
                           const std::function<void(const QString &, const QString &)> &done);
    // Calendar/contact change replay helpers.
    void postPimItem(const Akonadi::Item &item, const QString &path, const QJsonObject &body);
    // Contact photos live on a separate endpoint; upload after the JSON write, then commit.
    void putContactPhotoThenCommit(const Akonadi::Item &item);
    void patchPimItem(const Akonadi::Item &item, const QString &path, const QJsonObject &body);
    // Graph has no move API for events/tasks: recreate in the destination, delete the
    // original, then continue with the next item (sequential; commits when done).
    void movePimItem(const Akonadi::Item::List &items,
                     int index,
                     const std::shared_ptr<Akonadi::Item::List> &moved,
                     const Akonadi::Collection &source,
                     const Akonadi::Collection &destination);

    // Per-collection delta state (the @odata.deltaLink) is stored as a collection
    // attribute, exactly like EwsSyncStateAttribute.
    [[nodiscard]] static QString collectionDeltaLink(const Akonadi::Collection &col);
    void saveCollectionDeltaLink(Akonadi::Collection col, const QString &deltaLink);

    GraphClient mClient;
    QScopedPointer<GraphOAuth> mAuth;
    QScopedPointer<GraphSettings> mSettings;
    Akonadi::Collection mRootCollection;
    QString mFolderDeltaLink; // top-level mailFolders delta
    QString mSentFolderRemoteId; // resolved "Sent Items" folder id (sent reconciliation)
    QHash<QString, int> mSpecialFolderIndex; // remoteId -> kSpecialFolders index
    Akonadi::Collection::List mExtraCollections; // calendars + contacts
    QSet<QString> mKnownExtraIds; // to report server-side deletions (no delta for these)
    QTimer *mPollTimer = nullptr;
};
