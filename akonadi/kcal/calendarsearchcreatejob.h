#ifndef AKONADI_CALENDARSEARCHCREATEJOB_H
#define AKONADI_CALENDARSEARCHCREATEJOB_H

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
