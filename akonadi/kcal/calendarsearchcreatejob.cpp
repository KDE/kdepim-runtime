#include "calendarsearchcreatejob.h"
#include "calendarsearchinterface.h"

#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>

#include <KDateTime>
#include <KDebug>
#include <KLocalizedString>

#include <QtDBus/QDBusConnection>

using namespace Akonadi;

static QString dateToString( const KDateTime& dt ) {
  return dt.isValid() ? dt.toString() : QString();
}

class CalendarSearchCreateJob::Private {
    CalendarSearchCreateJob* const q;
public:
    explicit Private( CalendarSearchCreateJob* qq );
    void doStart();
    void searchCreated( const QVariantMap& result );
    void collectionFetched( KJob* job );
    Collection createdCollection;
    KDateTime startDate;
    KDateTime endDate;
    OrgFreedesktopAkonadiCalendarSearchAgentInterface* interface;
};

CalendarSearchCreateJob::Private::Private( CalendarSearchCreateJob* qq )
  : q( qq )
  , interface( new OrgFreedesktopAkonadiCalendarSearchAgentInterface( QLatin1String("org.freedesktop.Akonadi.Agent.akonadi_calendarsearch_agent"),
                 QLatin1String("/CalendarSearchAgent"),
                 QDBusConnection::sessionBus(),
                 q ) ) {
}

void CalendarSearchCreateJob::Private::doStart() {
    const QString startStr = dateToString( startDate );
    const QString endStr = dateToString( endDate );
    if ( !interface->callWithCallback( QLatin1String("createSearch"), QList<QVariant>() << startStr << endStr, q, SLOT(searchCreated(QVariantMap)) ) ) {
        q->setError( KJob::UserDefinedError );
        q->setErrorText( i18n( "Could not create search." ) );
        q->emitResult();
    }
}

void CalendarSearchCreateJob::Private::searchCreated( const QVariantMap& result ) {
    kDebug() << "search created";
    bool ok = false;
    const int error = result.value( QLatin1String("error") ).toInt();
    if ( error ) {
        q->setError( KJob::UserDefinedError );
        q->setErrorText( result.value( QLatin1String("errorString") ).toString() );
        q->emitResult();
        return;
    }
    const int id = result.value( QLatin1String("Collection") ).toInt( &ok );
    if ( !ok || id < 0 ) {
        q->setError( KJob::UserDefinedError );
        q->setErrorText( i18n("Could not parsed the collection ID") );
        q->emitResult();
        return;
    }

    Collection col;
    col.setId( id );
    CollectionFetchJob* job = new CollectionFetchJob( col );
    connect( job, SIGNAL(finished(KJob*)), q, SLOT(collectionFetched(KJob*)) );
}

void CalendarSearchCreateJob::Private::collectionFetched( KJob* j ) {
    const CollectionFetchJob* const job = qobject_cast<const CollectionFetchJob* const>( j );
    Q_ASSERT( job );
    if ( job->error() ) {
        q->setError( KJob::UserDefinedError );
        q->setErrorText( job->errorString() );
        q->emitResult();
        return;
    }
    if ( !job->collections().isEmpty() )
      createdCollection = job->collections().first();
    q->emitResult();
}

CalendarSearchCreateJob::CalendarSearchCreateJob( QObject* parent ) : KJob( parent ), d( new Private( this ) ) {
}

CalendarSearchCreateJob::~CalendarSearchCreateJob() {
    delete d;
}

void CalendarSearchCreateJob::setStartDate( const KDateTime& startDate ) {
    d->startDate = startDate;
}

void CalendarSearchCreateJob::setEndDate( const KDateTime& endDate ) {
    d->endDate = endDate;
}

void CalendarSearchCreateJob::start() {
    QMetaObject::invokeMethod( this, "doStart", Qt::QueuedConnection );
}

Collection CalendarSearchCreateJob::createdCollection() const {
    return d->createdCollection;
}
        
#include "calendarsearchcreatejob.moc"
