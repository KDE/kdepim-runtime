/*
    Copyright (c) 2009 KDAB
    Author: Frank Osterfeld <osterfeld@kde.org>

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

#include "incidencefilterproxymodel.h"

#include <akonadi/entitytreemodel.h>
#include <Akonadi/Item>

#include <KCal/Incidence>

using namespace Akonadi;
using namespace KCal;


class IncidenceFilterProxyModel::Private {
  public:
    explicit Private() : showEvents( true ), showJournals( true ), showTodos( true ) {}
    bool showEvents : 1;
    bool showJournals : 1;
    bool showTodos : 1;
};

class IncidenceFilterProxyModel::Visitor : public IncidenceBase::Visitor {
  IncidenceFilterProxyModel::Private* const d;
public:
  explicit Visitor( IncidenceFilterProxyModel::Private* dd ) : d( dd ), acceptLastIncidence( false ) {}

  /* reimp */ bool visit( Journal* );
  /* reimp */ bool visit( Todo* );
  /* reimp */ bool visit( Event* );
  /* reimp */ bool visit( FreeBusy* );

  bool acceptLastIncidence : 1;
};

bool IncidenceFilterProxyModel::Visitor::visit( Journal* ) {
  acceptLastIncidence = d->showJournals;
  return true;
}

bool IncidenceFilterProxyModel::Visitor::visit( Todo* ) {
  acceptLastIncidence = d->showTodos;
  return true;
}

bool IncidenceFilterProxyModel::Visitor::visit( Event* ) {
  acceptLastIncidence = d->showEvents;
  return true;
}

bool IncidenceFilterProxyModel::Visitor::visit( FreeBusy* ) {
  acceptLastIncidence = false;
  return true;
}

IncidenceFilterProxyModel::IncidenceFilterProxyModel( QObject* parent ) : QSortFilterProxyModel( parent ), d( new Private ) {
}

IncidenceFilterProxyModel::~IncidenceFilterProxyModel() {
  delete d;
}

bool IncidenceFilterProxyModel::showEvents() const {
  return d->showEvents;
}

void IncidenceFilterProxyModel::setShowEvents( bool show ) {
  if ( d->showEvents == show )
    return;
  d->showEvents = show;
  invalidateFilter();
}

bool IncidenceFilterProxyModel::showJournals() const {
  return d->showJournals;
}

void IncidenceFilterProxyModel::setShowJournals( bool show ) {
  if ( d->showJournals == show )
    return;
  d->showJournals = show;
  invalidateFilter();
}

bool IncidenceFilterProxyModel::showTodos() const {
  return d->showTodos;
}

void IncidenceFilterProxyModel::setShowTodos( bool show ) {
  if ( d->showTodos == show )
    return;
  d->showTodos = show;
  invalidateFilter();
}

void IncidenceFilterProxyModel::showAll() {
  if ( d->showEvents && d->showJournals && d->showTodos )
    return;
  d->showEvents = true;
  d->showJournals = true;
  d->showTodos = true;
  invalidateFilter();
}

void IncidenceFilterProxyModel::hideAll() {
  if ( !d->showEvents && !d->showJournals && !d->showTodos )
    return;
  d->showEvents = false;
  d->showJournals = false;
  d->showTodos = false;
  invalidateFilter();
}

bool IncidenceFilterProxyModel::filterAcceptsRow( int source_row, const QModelIndex& source_parent ) const {
  const QModelIndex idx = sourceModel()->index( source_row, 0, source_parent );
  if ( !idx.isValid() )
    return false;
  const Item item = idx.data( EntityTreeModel::ItemRole ).value<Item>();
  if ( !item.isValid() || !item.hasPayload<Incidence::Ptr>() )
    return false;
  const Incidence::Ptr inc = item.payload<Incidence::Ptr>();
  if ( !inc )
    return false;

  Visitor v( d );
  inc->accept( v );
  return v.acceptLastIncidence;
}
