/*
    This file is part of libkcal.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KCAL_RESOURCEAKONADI_P_H
#define KCAL_RESOURCEAKONADI_P_H

#include "subresource.h"
#include "resourceakonadi.h"
#include "sharedresourceprivate.h"

#include "kcal/kcalmimetypevisitor.h"

#include <kcal/assignmentvisitor.h>
#include <kcal/calendarlocal.h>

namespace Akonadi {
  class AgentFilterProxyModel;
  class AgentInstanceModel;
}

namespace KABC {
  class LockNull;
}

class KConfigGroup;

class KCal::ResourceAkonadi::Private : public SharedResourcePrivate<SubResource>,
                                       public KCal::Calendar::CalendarObserver
{
  Q_OBJECT

  public:
    explicit Private( ResourceAkonadi *parent );

    Private( const KConfigGroup &config, ResourceAkonadi *parent );

    ~Private();

    bool doSaveIncidence( Incidence *incidence );

    QString subResourceIdentifier( const QString &incidenceUid ) const;

  public:
    ResourceAkonadi *mParent;

    CalendarLocal mCalendar;

    KABC::LockNull *mLock;

    bool mInternalCalendarModification;

    AssignmentVisitor mIncidenceAssigner;

    Akonadi::KCalMimeTypeVisitor mMimeVisitor;

    Akonadi::AgentInstanceModel *mAgentModel;
    Akonadi::AgentFilterProxyModel *mAgentFilterModel;

  protected:
    bool openResource();

    bool closeResource();

    void clearResource();

    void subResourceAdded( SubResourceBase *subResource );

    void subResourceRemoved( SubResourceBase *subResource );

    void loadingResult( bool ok, const QString &errorString );

    void savingResult( bool ok, const QString &errorString );

    const SubResourceBase *storeSubResourceFromUser( const QString &uid, const QString &mimeType );

    Akonadi::Item createItem( const QString &kresId );

    Akonadi::Item updateItem( const Akonadi::Item &item, const QString &kresId, const QString &originalId );

    CollectionsByMimeType storeCollectionsFromOldDefault() const;

    // from the CalendarObserver interface
    void calendarIncidenceAdded( KCal::Incidence *incidence );

    void calendarIncidenceChanged( KCal::Incidence *incidence );

    void calendarIncidenceDeleted( KCal::Incidence *incidence );

  protected Q_SLOTS:
    void incidenceAdded( const IncidencePtr &incidencePtr, const QString &subResourceIdentifier );

    void incidenceChanged( const IncidencePtr &incidencePtr, const QString &subResourceIdentifier );

    void incidenceRemoved( const QString &uid, const QString &subResourceIdentifier );
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
