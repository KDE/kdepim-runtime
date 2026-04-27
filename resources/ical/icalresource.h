/*
    SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2009 David Jarvie <djarvie@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "settings.h"
#include "singlefileresource.h"

#include <KCalendarCore/FileStorage>
#include <KCalendarCore/IncidenceBase>
#include <KCalendarCore/MemoryCalendar>

class ICalResource : public Akonadi::SingleFileResource<SETTINGS_NAMESPACE::Settings>, public Akonadi::AgentBase::TagObserver
{
    Q_OBJECT

public:
    explicit ICalResource(const QString &id);
    ~ICalResource() override;

protected:
    [[nodiscard]] Akonadi::Collection rootCollection() const override;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    bool retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;
    void retrieveItems(const Akonadi::Collection &col) override;

    enum CheckType {
        CheckForAdded,
        CheckForChanged,
    };

    bool readFromFile(const QString &fileName) override;
    bool writeToFile(const QString &fileName) override;

    void aboutToQuit() override;

    /**
     * To be called at the start of derived class implementations of itemAdded()
     * or itemChanged() to verify that required conditions are true.
     * @param type the type of change to perform the checks for.
     * @return true if all checks are successful, and processing can continue;
     *         false if a check failed, in which case itemAdded() or itemChanged()
     *               should stop processing.
     */
    bool checkItemAddedChanged(const Akonadi::Item &item, CheckType type);

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &tagsAdded, const QSet<Akonadi::Tag> &tagsRemoved) override;
    void itemRemoved(const Akonadi::Item &item) override;
    void collectionChanged(const Akonadi::Collection &col) override;

    /** Return the local calendar. */
    KCalendarCore::MemoryCalendar::Ptr calendar() const;

private:
    KCalendarCore::MemoryCalendar::Ptr mCalendar;
    KCalendarCore::FileStorage::Ptr mFileStorage;
};
