/*
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

#ifndef KOLABRESOURCE_H
#define KOLABRESOURCE_H

#include "imapresourcebase.h"
#include <resourcestate.h>

class KolabResource : public ImapResourceBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Imap.Resource")

    using Akonadi::AgentBase::Observer::collectionChanged;

public:
    explicit KolabResource(const QString &id);
    ~KolabResource();

    QDialog *createConfigureDialog(WId windowId) override;
    Settings *settings() const override;
    void cleanup() override;

protected Q_SLOTS:
    void retrieveCollections() override;
    void delayedInit();

protected:
    ResourceStateInterface::Ptr createResourceState(const TaskArguments &) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemsMoved(const Akonadi::Item::List &item, const Akonadi::Collection &source,
                    const Akonadi::Collection &destination) override;
    //itemsRemoved and itemsFlags changed do not require translation, because they don't use the payload

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &parts) override;
    //collectionRemoved & collectionMoved do not require adjustments since they don't change the annotations

    void tagAdded(const Akonadi::Tag &tag) override;
    void tagChanged(const Akonadi::Tag &tag) override;
    void tagRemoved(const Akonadi::Tag &tag) override;
    void itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags) override;

    void itemsRelationsChanged(const Akonadi::Item::List &items,
                               const Akonadi::Relation::List &addedRelations,
                               const Akonadi::Relation::List &removedRelations) override;

    QString defaultName() const override;
    QByteArray clientId() const override;

private Q_SLOTS:
    void retrieveTags() override;
    void retrieveRelations() override;
    void onConfigurationDone(int result);
};

#endif
