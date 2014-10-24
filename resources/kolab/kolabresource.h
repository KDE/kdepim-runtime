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

    virtual KDialog *createConfigureDialog ( WId windowId );
    virtual Settings* settings() const;

protected Q_SLOTS:
    virtual void retrieveCollections();
    virtual void delayedInit();

protected:
    virtual ResourceStateInterface::Ptr createResourceState(const TaskArguments &);

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemsMoved( const Akonadi::Item::List &item, const Akonadi::Collection &source,
                           const Akonadi::Collection &destination );
    //itemsRemoved and itemsFlags changed do not require translation, because they don't use the payload

    virtual void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);
    virtual void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &parts);
    //collectionRemoved & collectionMoved do not require adjustments since they don't change the annotations

    virtual void tagAdded(const Akonadi::Tag &tag);
    virtual void tagChanged(const Akonadi::Tag &tag);
    virtual void tagRemoved(const Akonadi::Tag &tag);
    virtual void itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags);

    virtual QString defaultName();

private Q_SLOTS:
    void retrieveTags();
};

#endif
