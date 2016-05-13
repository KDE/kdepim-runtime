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

#ifndef DUMMYRESOURCESTATE_H
#define DUMMYRESOURCESTATE_H

#include <QtCore/QPair>
#include <QtCore/QVariant>

#include "resourcestateinterface.h"

typedef QPair<Akonadi::Tag::List, QHash<QString, Akonadi::Item::List> > TagListAndMembers;

class DummyResourceState : public ResourceStateInterface
{
public:
    typedef QSharedPointer<DummyResourceState> Ptr;

    explicit DummyResourceState();
    ~DummyResourceState();

    void setUserName(const QString &name);
    QString userName() const Q_DECL_OVERRIDE;

    void setResourceName(const QString &name);
    QString resourceName() const Q_DECL_OVERRIDE;

    void setResourceIdentifier(const QString &identifier);
    QString resourceIdentifier() const Q_DECL_OVERRIDE;

    void setServerCapabilities(const QStringList &capabilities);
    QStringList serverCapabilities() const Q_DECL_OVERRIDE;

    void setServerNamespaces(const QList<KIMAP::MailBoxDescriptor> &namespaces);
    QList<KIMAP::MailBoxDescriptor> serverNamespaces() const Q_DECL_OVERRIDE;
    QList<KIMAP::MailBoxDescriptor> personalNamespaces() const Q_DECL_OVERRIDE;
    QList<KIMAP::MailBoxDescriptor> userNamespaces() const Q_DECL_OVERRIDE;
    QList<KIMAP::MailBoxDescriptor> sharedNamespaces() const Q_DECL_OVERRIDE;

    void setAutomaticExpungeEnagled(bool enabled);
    bool isAutomaticExpungeEnabled() const Q_DECL_OVERRIDE;

    void setSubscriptionEnabled(bool enabled);
    bool isSubscriptionEnabled() const Q_DECL_OVERRIDE;
    void setDisconnectedModeEnabled(bool enabled);
    bool isDisconnectedModeEnabled() const Q_DECL_OVERRIDE;
    void setIntervalCheckTime(int interval);
    int intervalCheckTime() const Q_DECL_OVERRIDE;

    void setCollection(const Akonadi::Collection &collection);
    Akonadi::Collection collection() const Q_DECL_OVERRIDE;
    void setItem(const Akonadi::Item &item);
    Akonadi::Item item() const Q_DECL_OVERRIDE;
    Akonadi::Item::List items() const Q_DECL_OVERRIDE;

    void setParentCollection(const Akonadi::Collection &collection);
    Akonadi::Collection parentCollection() const Q_DECL_OVERRIDE;

    void setSourceCollection(const Akonadi::Collection &collection);
    Akonadi::Collection sourceCollection() const Q_DECL_OVERRIDE;
    void setTargetCollection(const Akonadi::Collection &collection);
    Akonadi::Collection targetCollection() const Q_DECL_OVERRIDE;

    void setParts(const QSet<QByteArray> &parts);
    QSet<QByteArray> parts() const Q_DECL_OVERRIDE;

    void setTag(const Akonadi::Tag &tag);
    Akonadi::Tag tag() const Q_DECL_OVERRIDE;
    void setAddedTags(const QSet<Akonadi::Tag> &addedTags);
    QSet<Akonadi::Tag> addedTags() const Q_DECL_OVERRIDE;
    void setRemovedTags(const QSet<Akonadi::Tag> &removedTags);
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

    QSet< QByteArray > addedFlags() const Q_DECL_OVERRIDE;
    QSet< QByteArray > removedFlags() const Q_DECL_OVERRIDE;

    void itemChangeCommitted(const Akonadi::Item &item) Q_DECL_OVERRIDE;
    void itemsChangesCommitted(const Akonadi::Item::List &items) Q_DECL_OVERRIDE;

    void collectionsRetrieved(const Akonadi::Collection::List &collections) Q_DECL_OVERRIDE;

    void collectionChangeCommitted(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

    void tagsRetrieved(const Akonadi::Tag::List &tags, const QHash<QString, Akonadi::Item::List> &) Q_DECL_OVERRIDE;
    void relationsRetrieved(const Akonadi::Relation::List &tags) Q_DECL_OVERRIDE;
    void tagChangeCommitted(const Akonadi::Tag &tag) Q_DECL_OVERRIDE;

    void searchFinished(const QVector<qint64> &result, bool isRid = true) Q_DECL_OVERRIDE;

    void changeProcessed() Q_DECL_OVERRIDE;

    void cancelTask(const QString &errorString) Q_DECL_OVERRIDE;
    void deferTask() Q_DECL_OVERRIDE;
    void restartItemRetrieval(Akonadi::Collection::Id col) Q_DECL_OVERRIDE;
    void taskDone() Q_DECL_OVERRIDE;

    void emitError(const QString &message) Q_DECL_OVERRIDE;
    void emitWarning(const QString &message) Q_DECL_OVERRIDE;
    void emitPercent(int percent) Q_DECL_OVERRIDE;

    void synchronizeCollectionTree() Q_DECL_OVERRIDE;
    void scheduleConnectionAttempt() Q_DECL_OVERRIDE;

    QChar separatorCharacter() const Q_DECL_OVERRIDE;
    void setSeparatorCharacter(const QChar &separator) Q_DECL_OVERRIDE;

    void showInformationDialog(const QString &message, const QString &title, const QString &dontShowAgainName) Q_DECL_OVERRIDE;

    int batchSize() const Q_DECL_OVERRIDE;
    void setItemMergingMode(Akonadi::ItemSync::MergeMode mergeMode) Q_DECL_OVERRIDE;

    MessageHelper::Ptr messageHelper() const Q_DECL_OVERRIDE;

    QList< QPair<QByteArray, QVariant> > calls() const;

private:
    void recordCall(const QByteArray callName, const QVariant &parameter = QVariant());

    QString m_userName;
    QString m_resourceName;
    QString m_resourceIdentifier;
    QStringList m_capabilities;
    QList<KIMAP::MailBoxDescriptor> m_namespaces;

    bool m_automaticExpunge;
    bool m_subscriptionEnabled;
    bool m_disconnectedMode;
    int m_intervalCheckTime;
    QChar m_separator;

    Akonadi::ItemSync::MergeMode m_mergeMode;

    Akonadi::Collection m_collection;
    Akonadi::Item::List m_items;

    Akonadi::Collection m_parentCollection;

    Akonadi::Collection m_sourceCollection;
    Akonadi::Collection m_targetCollection;

    QSet<QByteArray> m_parts;

    Akonadi::Tag m_tag;
    QSet<Akonadi::Tag> m_addedTags;
    QSet<Akonadi::Tag> m_removedTags;

    QList< QPair<QByteArray, QVariant> > m_calls;
};

#endif
