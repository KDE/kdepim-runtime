/*
    This file is part of libkcal.
    Copyright (c) 2008-2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "resourceakonadi.h"

#include "resourceakonadi_p.h"
#include "resourceakonadiconfig.h"

#include <kabc/locknull.h>
#include <kcal/calendarlocal.h>

#include <akonadi/agentfilterproxymodel.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancemodel.h>

#include <KDebug>

using namespace Akonadi;
using namespace KCal;

typedef boost::shared_ptr<Incidence> IncidencePtr;

ResourceAkonadi::ResourceAkonadi()
  : ResourceCalendar(), d( new Private( this ) )
{
}

ResourceAkonadi::ResourceAkonadi( const KConfigGroup &group )
  : ResourceCalendar( group ), d( new Private( group, this ) )
{
}

ResourceAkonadi::~ResourceAkonadi()
{
  delete d;
}

void ResourceAkonadi::writeConfig( KConfigGroup &group )
{
  ResourceCalendar::writeConfig( group );

  d->writeConfig( group );
}

void ResourceAkonadi::setStoreCollection( const Collection &collection )
{
  d->setDefaultStoreCollection( collection );
}

Collection ResourceAkonadi::storeCollection() const
{
  return d->defaultStoreCollection();
}

KABC::Lock *ResourceAkonadi::lock()
{
  return d->mLock;
}

bool ResourceAkonadi::addEvent( Event *event )
{
  const QString uid = event->uid();
  const QString mimeType = d->mMimeVisitor.mimeType( event );

  kDebug( 5800 ) << "Event (uid=" << uid
                 << ", summary=" << event->summary()
                 << ")";

  if ( d->addLocalItem( uid, mimeType ) ) {
    return d->mCalendar.addEvent( event );
  }

  return false;
}

bool ResourceAkonadi::deleteEvent( Event *event )
{
  return d->mCalendar.deleteEvent( event );
}

void ResourceAkonadi::deleteAllEvents()
{
  d->mCalendar.deleteAllEvents();
}

Event *ResourceAkonadi::event( const QString &uid )
{
  return d->mCalendar.event( uid );
}

Event::List ResourceAkonadi::rawEvents( EventSortField sortField,
                                        SortDirection sortDirection )
{
  return d->mCalendar.rawEvents( sortField, sortDirection );
}

Event::List ResourceAkonadi::rawEventsForDate( const QDate &date,
                                               const KDateTime::Spec &timespec,
                                               EventSortField sortField,
                                               SortDirection sortDirection )
{
  return d->mCalendar.rawEventsForDate( date, timespec, sortField, sortDirection );
}

Event::List ResourceAkonadi::rawEventsForDate( const KDateTime &date )
{
  return d->mCalendar.rawEventsForDate( date );
}

Event::List ResourceAkonadi::rawEvents( const QDate &start, const QDate &end,
                                        const KDateTime::Spec &timespec,
                                        bool inclusive )
{
  return d->mCalendar.rawEvents( start, end, timespec, inclusive );
}

bool ResourceAkonadi::addTodo( Todo *todo )
{
  const QString uid = todo->uid();
  const QString mimeType = d->mMimeVisitor.mimeType( todo );

  kDebug( 5800 ) << "Todo (uid=" << uid
                 << ", summary=" << todo->summary()
                 << ")";

  if ( d->addLocalItem( uid, mimeType ) ) {
    return d->mCalendar.addTodo( todo );
  }

  return false;
}

bool ResourceAkonadi::deleteTodo( Todo *todo )
{
  return d->mCalendar.deleteTodo( todo );
}

void ResourceAkonadi::deleteAllTodos()
{
  d->mCalendar.deleteAllTodos();
}

Todo *ResourceAkonadi::todo( const QString &uid )
{
  return d->mCalendar.todo( uid );
}

Todo::List ResourceAkonadi::rawTodos( TodoSortField sortField,
                                      SortDirection sortDirection )
{
  return d->mCalendar.rawTodos( sortField, sortDirection );
}

Todo::List ResourceAkonadi::rawTodosForDate( const QDate &date )
{
  return d->mCalendar.rawTodosForDate( date );
}

bool ResourceAkonadi::addJournal( Journal *journal )
{
  const QString uid = journal->uid();
  const QString mimeType = d->mMimeVisitor.mimeType( journal );

  kDebug( 5800 ) << "Journal (uid=" << uid
                 << ", summary=" << journal->summary()
                 << ")";

  if ( d->addLocalItem( uid, mimeType ) ) {
    return d->mCalendar.addJournal( journal );
  }

  return false;
}

bool ResourceAkonadi::deleteJournal( Journal *journal )
{
  return d->mCalendar.deleteJournal( journal );
}

void ResourceAkonadi::deleteAllJournals()
{
  d->mCalendar.deleteAllJournals();
}

Journal *ResourceAkonadi::journal( const QString &uid )
{
  return d->mCalendar.journal( uid );
}

Journal::List ResourceAkonadi::rawJournals( JournalSortField sortField,
                                            SortDirection sortDirection )
{
  return d->mCalendar.rawJournals( sortField, sortDirection );
}

Journal::List ResourceAkonadi::rawJournalsForDate( const QDate &date )
{
  return d->mCalendar.rawJournalsForDate( date );
}

Alarm::List ResourceAkonadi::alarms( const KDateTime &from, const KDateTime &to )
{
  return d->mCalendar.alarms( from, to );
}

Alarm::List ResourceAkonadi::alarmsTo( const KDateTime &to )
{
  return d->mCalendar.alarmsTo( to );
}

void ResourceAkonadi::setTimeSpec( const KDateTime::Spec &timeSpec )
{
  d->mCalendar.setTimeSpec( timeSpec );
}

KDateTime::Spec ResourceAkonadi::timeSpec() const
{
  return d->mCalendar.timeSpec();
}

void ResourceAkonadi::setTimeZoneId( const QString &timeZoneId )
{
  d->mCalendar.setTimeZoneId( timeZoneId );
}

QString ResourceAkonadi::timeZoneId() const
{
  return d->mCalendar.timeZoneId();
}

void ResourceAkonadi::shiftTimes( const KDateTime::Spec &oldSpec,
                                  const KDateTime::Spec &newSpec )
{
  d->mCalendar.shiftTimes( oldSpec, newSpec );
}

bool ResourceAkonadi::canHaveSubresources() const
{
  return true;
}

QString ResourceAkonadi::labelForSubresource( const QString &resource ) const
{
  kDebug(5800) << "resource=" << resource;

  QString label;
  SubResource *subResource = d->subResource( resource );
  if ( subResource != 0 ) {
    label = subResource->label();
  }

  return label;
}

void ResourceAkonadi::setSubresourceActive( const QString &subResource, bool active )
{
  kDebug(5800) << "subResource" << subResource << ", active" << active;

  // TODO might no longer be necessary
  bool changed = false;

  SubResource *resource = d->subResource( subResource );
  if ( resource != 0 ) {
    if ( active != resource->isActive() ) {
      resource->setActive( active );
      changed = true;
    }
  }

  if ( changed ) {
    emit resourceChanged( this );
  }
}

bool ResourceAkonadi::subresourceActive( const QString &resource ) const
{
  bool active = false;
  SubResource *subResource = d->subResource( resource );
  if ( subResource != 0 ) {
    active = subResource->isActive();
  }

  return active;
}

bool ResourceAkonadi::addSubresource( const QString &resource, const QString &parent )
{
  kDebug(5800) << "resource=" << resource << ", parent=" << parent;
  Q_ASSERT( !resource.isEmpty() );

  if ( parent.isEmpty() ) {
    kError(5800) << "Cannot create Akonadi toplevel collection";
    // TODO probably display a dialog working on the agent filter proxy model
    // and then create and name the resource appropriately.
    return false;
  }

  SubResource *subResource = d->subResource( parent );
  if ( subResource == 0 ) {
    kError(5800) << "No such parent subresource/collection:" << parent;
    return false;
  }

  return subResource->createChildSubResource( resource );
}

bool ResourceAkonadi::removeSubresource( const QString &resource )
{
  kDebug(5800) << "resource=" << resource;
  Q_ASSERT( !resource.isEmpty() );

  SubResource *subResource = d->subResource( resource );
  if ( subResource == 0 ) {
    kError(5800) << "No such subresource: " << resource;
    return false;
  }

  return subResource->remove();
}

QString ResourceAkonadi::subresourceType( const QString &resource )
{
  kDebug(5800) << "resource=" << resource;

  QString type;
  SubResource *subResource = d->subResource( resource );
  if ( subResource != 0 ) {
    type = subResource->subResourceType();
  }

  return type;
}

QString ResourceAkonadi::subresourceIdentifier( Incidence *incidence )
{
  return d->subResourceIdentifier( incidence->uid() );
}

QStringList ResourceAkonadi::subresources() const
{
  kDebug(5800) << d->subResourceIdentifiers();
  return d->subResourceIdentifiers();
}

QString ResourceAkonadi::infoText() const
{
  const QString online  = i18nc( "access to the source's backend possible", "Online" );
  const QString offline = i18nc( "currently no access to the source's backend possible",
                                 "Offline" );
  const QLatin1String br( "<br>" );

  QString text = i18nc( "@info:tooltip visible name of the resource",
                        "<title>%1</title>", resourceName() );
  text += i18nc( "@info:tooltip resource type", "Type: Akonadi Calendar Resource" ) + br;

  const int rowCount = d->mAgentFilterModel->rowCount();
  for ( int row = 0; row < rowCount; ++row ) {
    QModelIndex index = d->mAgentFilterModel->index( row, 0 );
    if ( index.isValid() ) {
      QVariant data = d->mAgentFilterModel->data( index, AgentInstanceModel::InstanceRole );
      if ( data.isValid() ) {
        AgentInstance instance = data.value<AgentInstance>();
        if ( instance.isValid() ) {
          // TODO probably add progress if "Running"
          QString status = instance.statusMessage();

          text += br;
          text += i18nc( "@info:tooltip name of a calendar data source",
                         "<b>%1</b>", instance.name() ) + br;
          text += i18nc( "@info:tooltip status of a calendar data source and its "
                         "online/offline state",
                         "Status: %1 (%2)", status,
                         ( instance.isOnline() ? online : offline ) ) + br;
        }
      }
    }
  }

  return text;
}

bool ResourceAkonadi::doLoad( bool syncCache )
{
  kDebug(5800) << "syncCache=" << syncCache;

  d->clear();
  return d->load();
}

bool ResourceAkonadi::doSave( bool syncCache )
{
  kDebug(5800) << "syncCache=" << syncCache;

  return d->doSave();
}

bool ResourceAkonadi::doSave( bool syncCache, Incidence *incidence )
{
  kDebug(5800) << "syncCache=" << syncCache
               << ", incidence" << incidence->uid();

  return d->doSaveIncidence( incidence );
}

bool ResourceAkonadi::doOpen()
{
  return d->doOpen();
}

void ResourceAkonadi::doClose()
{
  d->clear();
  d->doClose();
}

#include "resourceakonadi.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
