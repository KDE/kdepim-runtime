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
#include "googleresource.h"

GenericHandler::GenericHandler(GoogleResource *resource, GoogleSettings *settings)
    : m_resource(resource)
    , m_settings(settings)
{
}

GenericHandler::~GenericHandler() = default;

void GenericHandler::emitReadyStatus()
{
    Q_EMIT status(Akonadi::AgentBase::Idle, i18nc("@info:status", "Ready"));
}

void GenericHandler::itemsLinked(const Akonadi::Item::List &/*items*/, const Akonadi::Collection &/*collection*/)
{
    m_resource->cancelTask(i18n("Cannot handle item linking"));
}

void GenericHandler::itemsUnlinked(const Akonadi::Item::List &/*items*/, const Akonadi::Collection &/*collection*/)
{
    m_resource->cancelTask(i18n("Cannot handle item unlinking"));
}

#include "moc_generichandler.cpp"
