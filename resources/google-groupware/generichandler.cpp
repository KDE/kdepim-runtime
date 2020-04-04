/*
    Copyright (C) 2020  Igor Poboiko <igor.poboiko@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "generichandler.h"
#include "googleresourcestateinterface.h"
#include "googleresource_debug.h"

#include <AkonadiAgentBase/AgentBase>

#include <KGAPI/Job>

GenericHandler::GenericHandler(GoogleResourceStateInterface *iface, GoogleSettings *settings)
    : m_iface(iface)
    , m_settings(settings)
{
}

GenericHandler::~GenericHandler() = default;

void GenericHandler::itemsLinked(const Akonadi::Item::List &/*items*/, const Akonadi::Collection &/*collection*/)
{
    m_iface->cancelTask(i18n("Cannot handle item linking"));
}

void GenericHandler::itemsUnlinked(const Akonadi::Item::List &/*items*/, const Akonadi::Collection &/*collection*/)
{
    m_iface->cancelTask(i18n("Cannot handle item unlinking"));
}

void GenericHandler::slotGenericJobFinished(KGAPI2::Job *job)
{
    if (!m_iface->handleError(job)) {
        return;
    }
    if (job->property(ITEM_PROPERTY).isValid()) {
        qCDebug(GOOGLE_LOG) << "Item change committed";
        m_iface->itemChangeCommitted(job->property(ITEM_PROPERTY).value<Akonadi::Item>());
    } else if (job->property(ITEMS_PROPERTY).isValid()) {
        qCDebug(GOOGLE_LOG) << "Items changes committed";
        m_iface->itemsChangesCommitted(job->property(ITEMS_PROPERTY).value<Akonadi::Item::List>());
    } else if (job->property(COLLECTION_PROPERTY).isValid()) {
        qCDebug(GOOGLE_LOG) << "Collection change committed";
        m_iface->collectionChangeCommitted(job->property(COLLECTION_PROPERTY).value<Akonadi::Collection>());
    } else {
        qCDebug(GOOGLE_LOG) << "Task done";
        m_iface->taskDone();
    }

    emitReadyStatus();
}

void GenericHandler::emitReadyStatus()
{
    m_iface->emitStatus(Akonadi::AgentBase::Idle, i18nc("@status", "Ready"));
}

#include "moc_generichandler.cpp"
