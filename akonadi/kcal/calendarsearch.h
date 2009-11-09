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
#ifndef AKONADI_CALENDARSEARCH_H
#define AKONADI_CALENDARSEARCH_H

#include "akonadi-kcal_export.h"

#include <Akonadi/Collection>

#include <QObject>

class QAbstractItemModel;
class QItemSelection;
class QItemSelectionModel;

class KDateTime;

namespace Akonadi {

    class CollectionSelectionProxyModel;

    class AKONADI_KCAL_EXPORT CalendarSearch : public QObject
    {
        Q_OBJECT
    public:
        enum Error {
            NoError=0,
            SomeError=1
        };

        explicit CalendarSearch( QObject* parent=0 );
        ~CalendarSearch();

        QAbstractItemModel* model() const;

        KDateTime startDate() const;

        KDateTime endDate() const;

        QString errorString() const;
        Error error() const;
        bool hasError() const;

        QItemSelectionModel* selectionModel() const;
        void setSelectionModel( QItemSelectionModel* selectionModel );

    public Q_SLOTS:
        void setStartDate( const KDateTime& startDate );
        void setEndDate( const KDateTime& endDate );

    Q_SIGNALS:
        void errorOccurred();

    private:
        class Private;
        Private* const d;
        Q_PRIVATE_SLOT( d, void updateSearch() )
        Q_PRIVATE_SLOT( d, void collectionSelectionChanged( const QItemSelection &, const QItemSelection & ) )
//        Q_PRIVATE_SLOT( d, void searchCreated( const QVariantMap & ) )
//        Q_PRIVATE_SLOT( d, void collectionFetched( KJob * ) )
    };
}

#endif // AKONADI_CALENDARSEARCH_H
