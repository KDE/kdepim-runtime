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
#ifndef AKONADI_CALENDARSEARCHCREATEJOB_H
#define AKONADI_CALENDARSEARCHCREATEJOB_H

#include <Akonadi/Collection>

#include <KJob>

class KDateTime;

namespace Akonadi {
    class Collection;

    class CalendarSearchCreateJob : public KJob
    {
    Q_OBJECT
    public:
        explicit CalendarSearchCreateJob( QObject* parent=0 );
        ~CalendarSearchCreateJob();

        void setStartDate( const KDateTime& startDate );
        void setEndDate( const KDateTime& endDate );

        Collection::List sourceCollections() const;
        void setSourceCollections( const Collection::List& collections );

        /**
         * Reimplemented from KJob
         */
        void start();

        Collection createdCollection() const;

    private:
        class Private;
        Private* const d;
        Q_PRIVATE_SLOT( d, void doStart() )
        Q_PRIVATE_SLOT( d, void searchCreated( const QVariantMap & ) )
        Q_PRIVATE_SLOT( d, void collectionFetched( KJob * ) )
    };
}

#endif // AKONADI_CALENDARSEARCHCREATEJOB_H
