/*
    Copyright (C) 2011-2013  Daniel Vrátil <dvratil@redhat.com>

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

#ifndef GOOGLE_CALENDAR_CALENDARRESOURCE_H
#define GOOGLE_CALENDAR_CALENDARRESOURCE_H

#include "common/googleresource.h"

#include <AkonadiCore/Item>
#include <AkonadiCore/Collection>
#include <Akonadi/Calendar/FreeBusyProviderBase>

class CalendarResource : public GoogleResource, public Akonadi::FreeBusyProviderBase
{
    Q_OBJECT
public:
    explicit CalendarResource(const QString &id);
    ~CalendarResource() override;

public:
    using GoogleResource::collectionChanged; // So we don't trigger -Woverloaded-virtual
    GoogleSettings *settings() const override;
    QList< QUrl > scopes() const override;

protected:
    // Freebusy
    QDateTime lastCacheUpdate() const override;
    void canHandleFreeBusy(const QString &email) const override;
    void retrieveFreeBusy(const QString &email, const QDateTime &start, const QDateTime &end) override;

protected:
    using ResourceBase::retrieveItems; // Suppress -Woverload-virtual

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers) override;
    void itemRemoved(const Akonadi::Item &item) override;
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

    void slotItemsRetrieved(KGAPI2::Job *job);
    void slotCollectionsRetrieved(KGAPI2::Job *job);
    void slotCalendarsRetrieved(KGAPI2::Job *job);
    void slotRemoveTaskFetchJobFinished(KJob *job);
    void slotDoRemoveTask(KJob *job);
    void slotModifyTaskReparentFinished(KGAPI2::Job *job);
    void slotTaskAddedSearchFinished(KJob *);
    void slotCreateJobFinished(KGAPI2::Job *job);

    void slotCanHandleFreeBusyJobFinished(KGAPI2::Job *job);
    void slotRetrieveFreeBusyJobFinished(KGAPI2::Job *job);

protected:
    int runConfigurationDialog(WId windowId) override;
    void updateResourceName() override;

private:
    QMap<QString, Akonadi::Collection> m_collections;
    Akonadi::Collection m_rootCollection;
};

#endif // CALENDARRESOURCE_H
