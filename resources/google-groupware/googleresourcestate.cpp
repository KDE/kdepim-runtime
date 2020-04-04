/*
    Copyright (c) 2020 Igor Poboiko <igor.poboiko@gmail.com>

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

#include "googleresourcestate.h"
#include "googleresource.h"

using namespace Akonadi;

GoogleResourceState::GoogleResourceState(GoogleResource *resource)
    : m_resource(resource)
{
}

// Items handling
void GoogleResourceState::itemRetrieved(const Item &item)
{
    m_resource->itemRetrieved(item);
}

void GoogleResourceState::itemsRetrieved(const Item::List &items)
{
    m_resource->itemsRetrieved(items);
}

void GoogleResourceState::itemsRetrievedIncremental(const Item::List &changed, const Item::List &removed)
{
    m_resource->itemsRetrievedIncremental(changed, removed);
}

void GoogleResourceState::itemsRetrievalDone()
{
    m_resource->itemsRetrievalDone();
}

void GoogleResourceState::setTotalItems(int amount)
{
    m_resource->setTotalItems(amount);

}
void GoogleResourceState::itemChangeCommitted(const Item &item)
{
    m_resource->changeCommitted(item);
}

void GoogleResourceState::itemsChangesCommitted(const Item::List &items)
{
    m_resource->changesCommitted(items);
}

Item::List GoogleResourceState::currentItems()
{
    return m_resource->currentItems();
}

// Collections handling
void GoogleResourceState::collectionsRetrieved(const Collection::List &collections)
{
    m_resource->collectionsRetrieved(collections);
}

void GoogleResourceState::collectionAttributesRetrieved(const Collection &collection)
{
    m_resource->collectionAttributesRetrieved(collection);
}

void GoogleResourceState::collectionChangeCommitted(const Collection &collection)
{
    m_resource->changeCommitted(collection);
}

void GoogleResourceState::collectionsRetrievedFromHandler(const Collection::List &collections)
{
    m_resource->collectionsRetrievedFromHandler(collections);
}

Collection GoogleResourceState::currentCollection()
{
    return m_resource->currentCollection();
}

// Tags handling
void GoogleResourceState::tagsRetrieved(const Tag::List &tags, const QHash<QString, Item::List> &tagMembers)
{
    m_resource->tagsRetrieved(tags, tagMembers);
}

void GoogleResourceState::tagChangeCommitted(const Tag &tag)
{
    m_resource->changeCommitted(tag);
}


// Relations handling
void GoogleResourceState::relationsRetrieved(const Relation::List &relations)
{
    m_resource->relationsRetrieved(relations);
}


// FreeBusy handling
void GoogleResourceState::freeBusyRetrieved(const QString &email, const QString &freeBusy, bool success, const QString &errorText = QString())
{
    m_resource->freeBusyRetrieved(email, freeBusy, success, errorText);
}

void GoogleResourceState::handlesFreeBusy(const QString &email, bool handles)
{
    m_resource->handlesFreeBusy(email, handles);
}


// Result reporting
void GoogleResourceState::changeProcessed()
{
    m_resource->changeProcessed();
}

void GoogleResourceState::cancelTask(const QString &errorString)
{
    m_resource->cancelTask(errorString);
}

void GoogleResourceState::deferTask()
{
    m_resource->deferTask();
}

void GoogleResourceState::taskDone()
{
    m_resource->taskDone();
}

void GoogleResourceState::emitStatus(int status, const QString &message)
{
    Q_EMIT m_resource->status(status, message);
}

void GoogleResourceState::emitError(const QString &message)
{
    Q_EMIT m_resource->error(message);
}

void GoogleResourceState::emitWarning(const QString &message)
{
    Q_EMIT m_resource->warning(message);
}

void GoogleResourceState::emitPercent(int percent)
{
    Q_EMIT m_resource->percent(percent);
}

bool GoogleResourceState::canPerformTask()
{
    return m_resource->canPerformTask();
}

bool GoogleResourceState::handleError(KGAPI2::Job *job, bool _cancelTask)
{
    return m_resource->handleError(job, _cancelTask);
}

void GoogleResourceState::scheduleCustomTask(QObject *receiver, const char *method, const QVariant &argument)
{
    return m_resource->scheduleCustomTask(receiver, method, argument);
}
