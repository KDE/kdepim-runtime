/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CALENDARHANDLER_H
#define CALENDARHANDLER_H

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>
#include <KCalendarCore/Incidence>

#include "etesyncadapter.h"
#include "etesyncclientstate.h"

class EteSyncResource;

class CalendarHandler : public QObject
{
    Q_OBJECT
public:
    typedef std::unique_ptr<CalendarHandler> Ptr;
    CalendarHandler(EteSyncResource *resource);

    void setupItems(EteSyncEntry **entries, Akonadi::Collection &collection);

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection);
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts);
    void itemRemoved(const Akonadi::Item &item);

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);
    void collectionChanged(const Akonadi::Collection &collection);
    void collectionRemoved(const Akonadi::Collection &collection);

protected:
    QString getLocalCalendar(const QString &incidenceUid) const;

    bool updateLocalCalendar(const KCalendarCore::Incidence::Ptr &incidence);

    void deleteLocalCalendar(const KCalendarCore::Incidence::Ptr &incidence);

    QString baseDirectoryPath() const;

private:
    EteSyncResource *mResource = nullptr;
    EteSyncClientState *mClientState = nullptr;
};

#endif
