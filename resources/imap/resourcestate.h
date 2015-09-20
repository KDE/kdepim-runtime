/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef RESOURCESTATE_H
#define RESOURCESTATE_H

#include "resourcestateinterface.h"
#include "messagehelper.h"

class ImapResourceBase;

struct TaskArguments {
    TaskArguments() {}
    TaskArguments(const Akonadi::Item &_item): items(Akonadi::Item::List() << _item) {}
    TaskArguments(const Akonadi::Item &_item, const Akonadi::Collection &_collection): collection(_collection), items(Akonadi::Item::List() << _item) {}
    TaskArguments(const Akonadi::Item &_item, const QSet<QByteArray> &_parts): items(Akonadi::Item::List() << _item), parts(_parts) {}
    TaskArguments(const Akonadi::Item::List &_items): items(_items) {}
    TaskArguments(const Akonadi::Item::List &_items, const QSet<QByteArray> &_addedFlags, const QSet<QByteArray> &_removedFlags): items(_items), addedFlags(_addedFlags), removedFlags(_removedFlags) {}
    TaskArguments(const Akonadi::Item::List &_items, const Akonadi::Collection &_sourceCollection, const Akonadi::Collection &_targetCollection): items(_items), sourceCollection(_sourceCollection), targetCollection(_targetCollection) {}
    TaskArguments(const Akonadi::Item::List &_items, const QSet<Akonadi::Tag> &_addedTags, const QSet<Akonadi::Tag> &_removedTags): items(_items), addedTags(_addedTags), removedTags(_removedTags) {}
    TaskArguments(const Akonadi::Item::List &_items, const Akonadi::Relation::List &_addedRelations, const Akonadi::Relation::List &_removedRelations): items(_items), addedRelations(_addedRelations), removedRelations(_removedRelations) {}
    TaskArguments(const Akonadi::Collection &_collection): collection(_collection) {}
    TaskArguments(const Akonadi::Collection &_collection, const Akonadi::Collection &_parentCollection): collection(_collection), parentCollection(_parentCollection) {}
    TaskArguments(const Akonadi::Collection &_collection, const Akonadi::Collection &_sourceCollection, const Akonadi::Collection &_targetCollection): collection(_collection), sourceCollection(_sourceCollection), targetCollection(_targetCollection) {}
    TaskArguments(const Akonadi::Collection &_collection, const QSet<QByteArray> &_parts): collection(_collection), parts(_parts) {}
    TaskArguments(const Akonadi::Tag &_tag) : tag(_tag) {}
    Akonadi::Collection collection;
    Akonadi::Item::List items;
    Akonadi::Collection parentCollection; //only used as parent of a collection
    Akonadi::Collection sourceCollection;
    Akonadi::Collection targetCollection;
    Akonadi::Tag tag;
    QSet<QByteArray> parts;
    QSet<QByteArray> addedFlags;
    QSet<QByteArray> removedFlags;
    QSet<Akonadi::Tag> addedTags;
    QSet<Akonadi::Tag> removedTags;
    Akonadi::Relation::List addedRelations;
    Akonadi::Relation::List removedRelations;
};

class ResourceState : public ResourceStateInterface
{
public:
    explicit ResourceState(ImapResourceBase *resource, const TaskArguments &arguments);

public:
    ~ResourceState();

    QString userName() const Q_DECL_OVERRIDE;
    QString resourceName() const Q_DECL_OVERRIDE;
    QString resourceIdentifier() const Q_DECL_OVERRIDE;
    QStringList serverCapabilities() const Q_DECL_OVERRIDE;
    QList<KIMAP::MailBoxDescriptor> serverNamespaces() const Q_DECL_OVERRIDE;
    QList<KIMAP::MailBoxDescriptor> personalNamespaces() const Q_DECL_OVERRIDE;
    QList<KIMAP::MailBoxDescriptor> userNamespaces() const Q_DECL_OVERRIDE;
    QList<KIMAP::MailBoxDescriptor> sharedNamespaces() const Q_DECL_OVERRIDE;

    bool isAutomaticExpungeEnabled() const Q_DECL_OVERRIDE;
    bool isSubscriptionEnabled() const Q_DECL_OVERRIDE;
    bool isDisconnectedModeEnabled() const Q_DECL_OVERRIDE;
    int intervalCheckTime() const Q_DECL_OVERRIDE;

    Akonadi::Collection collection() const Q_DECL_OVERRIDE;
    Akonadi::Item item() const Q_DECL_OVERRIDE;
    Akonadi::Item::List items() const Q_DECL_OVERRIDE;

    Akonadi::Collection parentCollection() const Q_DECL_OVERRIDE;

    Akonadi::Collection sourceCollection() const Q_DECL_OVERRIDE;
    Akonadi::Collection targetCollection() const Q_DECL_OVERRIDE;

    QSet<QByteArray> parts() const Q_DECL_OVERRIDE;
    QSet<QByteArray> addedFlags() const Q_DECL_OVERRIDE;
    QSet<QByteArray> removedFlags() const Q_DECL_OVERRIDE;

    Akonadi::Tag tag() const Q_DECL_OVERRIDE;
    QSet<Akonadi::Tag> addedTags() const Q_DECL_OVERRIDE;
    QSet<Akonadi::Tag> removedTags() const Q_DECL_OVERRIDE;

    Akonadi::Relation::List addedRelations() const Q_DECL_OVERRIDE;
    Akonadi::Relation::List removedRelations() const Q_DECL_OVERRIDE;

    QString rootRemoteId() const Q_DECL_OVERRIDE;

    void setIdleCollection(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void applyCollectionChanges(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

    void collectionAttributesRetrieved(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

    void itemRetrieved(const Akonadi::Item &item) Q_DECL_OVERRIDE;

    void itemsRetrieved(const Akonadi::Item::List &items) Q_DECL_OVERRIDE;
    void itemsRetrievedIncremental(const Akonadi::Item::List &changed, const Akonadi::Item::List &removed) Q_DECL_OVERRIDE;
    void itemsRetrievalDone() Q_DECL_OVERRIDE;

    void setTotalItems(int) Q_DECL_OVERRIDE;

    void itemChangeCommitted(const Akonadi::Item &item) Q_DECL_OVERRIDE;
    void itemsChangesCommitted(const Akonadi::Item::List &items) Q_DECL_OVERRIDE;

    void collectionsRetrieved(const Akonadi::Collection::List &collections) Q_DECL_OVERRIDE;

    void tagsRetrieved(const Akonadi::Tag::List &tags, const QHash<QString, Akonadi::Item::List> &) Q_DECL_OVERRIDE;
    void relationsRetrieved(const Akonadi::Relation::List &tags) Q_DECL_OVERRIDE;

    void collectionChangeCommitted(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

    void tagChangeCommitted(const Akonadi::Tag &tag) Q_DECL_OVERRIDE;

    void changeProcessed() Q_DECL_OVERRIDE;

    void searchFinished(const QVector<qint64> &result, bool isRid = true) Q_DECL_OVERRIDE;

    void cancelTask(const QString &errorString) Q_DECL_OVERRIDE;
    void deferTask() Q_DECL_OVERRIDE;
    void restartItemRetrieval(Akonadi::Collection::Id col) Q_DECL_OVERRIDE;
    void taskDone() Q_DECL_OVERRIDE;

    void emitError(const QString &message) Q_DECL_OVERRIDE;
    void emitWarning(const QString &message) Q_DECL_OVERRIDE;

    void emitPercent(int percent) Q_DECL_OVERRIDE;

    virtual void synchronizeCollection(Akonadi::Collection::Id);
    void synchronizeCollectionTree() Q_DECL_OVERRIDE;
    void scheduleConnectionAttempt() Q_DECL_OVERRIDE;

    QChar separatorCharacter() const Q_DECL_OVERRIDE;
    void setSeparatorCharacter(const QChar &separator) Q_DECL_OVERRIDE;

    void showInformationDialog(const QString &message, const QString &title, const QString &dontShowAgainName) Q_DECL_OVERRIDE;

    int batchSize() const Q_DECL_OVERRIDE;

    MessageHelper::Ptr messageHelper() const Q_DECL_OVERRIDE;

    void setItemMergingMode(Akonadi::ItemSync::MergeMode mergeMode) Q_DECL_OVERRIDE;

private:
    ImapResourceBase *m_resource;
    const TaskArguments m_arguments;
};

#endif
