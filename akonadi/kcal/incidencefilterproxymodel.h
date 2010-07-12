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
#ifndef AKONADI_KCAL_INCIDENCEFILTERPROXYMODEL_H
#define AKONADI_KCAL_INCIDENCEFILTERPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>

namespace KCalCore {
  class CalFilter;
}

class KDateTime;
namespace Akonadi {
  class IncidenceFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    explicit IncidenceFilterProxyModel( QObject* parent=0 );
    ~IncidenceFilterProxyModel();

    bool showJournals() const;
    void setShowJournals( bool show );

    bool showEvents() const;
    void setShowEvents( bool show );

    bool showTodos() const;
    void setShowTodos( bool show );

    void showAll();
    void hideAll();

  protected:
    /* reimp */ bool filterAcceptsRow( int source_row, const QModelIndex& source_parent ) const;
    
  private:
    class Visitor;
    friend class Akonadi::IncidenceFilterProxyModel::Visitor;
    class Private;
    Private* const d;
  };
}

#endif // AKONADI_KCAL_INCIDENCEFILTERPROXYMODEL_H
