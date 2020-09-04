/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef CALENDARTASKBASEHANDLER_H
#define CALENDARTASKBASEHANDLER_H

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>
#include <KCalendarCore/Incidence>

#include "basehandler.h"
#include "etesyncadapter.h"
#include "etesyncclientstate.h"

class EteSyncResource;

class CalendarTaskBaseHandler : public BaseHandler
{
    Q_OBJECT
public:
    explicit CalendarTaskBaseHandler(EteSyncResource *resource);

    void getItemListFromEntries(std::vector<EteSyncEntryPtr> &entries, Item::List &changedItems, Item::List &removedItems, Collection &collection, const QString &journalUid, QString &prevUid) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

protected:
    QString getLocalCalendar(const QString &incidenceUid) const;
    bool updateLocalCalendar(const KCalendarCore::Incidence::Ptr &incidence);
    void deleteLocalCalendar(const QString &incidenceUid);

    QString baseDirectoryPath() const override;
};

#endif
