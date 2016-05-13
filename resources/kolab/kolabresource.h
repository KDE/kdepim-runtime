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

#include "imapresource.h"
#include <resourcestate.h>

class KolabResource : public ImapResource
{
    Q_OBJECT

    using Akonadi::AgentBase::Observer::collectionChanged;

public:
    explicit KolabResource(const QString &id);
    ~KolabResource();

    QDialog *createConfigureDialog(WId windowId) Q_DECL_OVERRIDE;
    Settings *settings() const Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void retrieveCollections() Q_DECL_OVERRIDE;
    void delayedInit();

protected:
    ResourceStateInterface::Ptr createResourceState(const TaskArguments &) Q_DECL_OVERRIDE;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void itemsMoved(const Akonadi::Item::List &item, const Akonadi::Collection &source,
                    const Akonadi::Collection &destination) Q_DECL_OVERRIDE;
    //itemsRemoved and itemsFlags changed do not require translation, because they don't use the payload

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) Q_DECL_OVERRIDE;
    void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    //collectionRemoved & collectionMoved do not require adjustments since they don't change the annotations

    void tagAdded(const Akonadi::Tag &tag) Q_DECL_OVERRIDE;
    void tagChanged(const Akonadi::Tag &tag) Q_DECL_OVERRIDE;
    void tagRemoved(const Akonadi::Tag &tag) Q_DECL_OVERRIDE;
    void itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags) Q_DECL_OVERRIDE;

    void itemsRelationsChanged(const Akonadi::Item::List &items,
                               const Akonadi::Relation::List &addedRelations,
                               const Akonadi::Relation::List &removedRelations) Q_DECL_OVERRIDE;

    QString defaultName() const Q_DECL_OVERRIDE;
    QByteArray clientId() const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void retrieveTags() Q_DECL_OVERRIDE;
    void retrieveRelations() Q_DECL_OVERRIDE;
};

#endif
