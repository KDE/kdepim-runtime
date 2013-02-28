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
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GOOGLE_CALENDAR_CALENDARRESOURCE_H
#define GOOGLE_CALENDAR_CALENDARRESOURCE_H

#include "common/googleresource.h"

#include <Akonadi/Item>
#include <Akonadi/Collection>

class CalendarResource : public GoogleResource
{
  Q_OBJECT
  public:
    explicit CalendarResource( const QString &id );
    ~CalendarResource();

  public:
    virtual GoogleSettings *settings() const;
    virtual QList< QUrl > scopes() const;

  protected Q_SLOTS:
    virtual void retrieveCollections();
    virtual void retrieveItems( const Akonadi::Collection &collection );

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet< QByteArray >& partIdentifiers );
    virtual void itemRemoved( const Akonadi::Item &item );
    virtual void itemMoved( const Akonadi::Item &item,
                            const Akonadi::Collection &collectionSource,
                            const Akonadi::Collection &collectionDestination );

    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionRemoved( const Akonadi::Collection &collection );

    void slotItemsRetrieved( KGAPI2::Job *job );
    void slotCollectionsRetrieved( KGAPI2::Job *job );
    void slotCalendarsRetrieved( KGAPI2::Job *job );
    void slotRemoveTaskFetchJobFinished( KJob *job );
    void slotDoRemoveTask( KJob *job );
    void slotModifyTaskReparentFinished( KGAPI2::Job *job );
    void slotTaskAddedSearchFinished( KJob * );
    void slotCreateJobFinished( KGAPI2::Job *job );

  protected:
    virtual int runConfigurationDialog( WId windowId );
    virtual void updateResourceName();

  private:
    QMap<QString, Akonadi::Collection> m_collections;
    Akonadi::Collection m_rootCollection;

};

#endif // CALENDARRESOURCE_H
