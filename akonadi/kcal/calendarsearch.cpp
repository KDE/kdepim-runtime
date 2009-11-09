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
#include "calendarsearch.h"
#include "calendarmodel.h"
#include "calendarsearchinterface.h"
#include "kcalmimetypevisitor.h"
#include "daterangefilterproxymodel.h"
#include "utils.h"

#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/EntityDisplayAttribute>
#include <akonadi/entitymimetypefiltermodel.h>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Session>

#include <KDateTime>
#include <KDebug>
#include <KLocalizedString>
#include <KRandom>
#include <kselectionproxymodel.h>

#include <QItemSelection>
#include <QItemSelectionModel>
#include <QSet>
#include <QTimer>
#include <QtDBus/QDBusConnection>

using namespace Akonadi;

static QString dateToString( const KDateTime& dt ) {
  return dt.isValid() ? dt.toString() : QString();
}

class CalendarSearch::Private {
    CalendarSearch* const q;
public:
    explicit Private( CalendarSearch* qq );
    ~Private() {
        //TODO delete created search collection
    }

    void updateSearch();
    void triggerDelayedUpdate();
    void searchCreated( const QVariantMap& result );
    void collectionSelectionChanged( const QItemSelection &, const QItemSelection & );
    void collectionFetched( KJob* job );
    Collection createdCollection;
    KDateTime startDate;
    KDateTime endDate;
    QTimer updateTimer;
    OrgFreedesktopAkonadiCalendarSearchAgentInterface* interface;
    QString errorString;
    CalendarSearch::Error error;
    CalendarModel* calendarModel;
    EntityMimeTypeFilterModel* filterProxy;
    KSelectionProxyModel* selectionProxyModel;
    DateRangeFilterProxyModel* dateRangeProxyModel;
    QItemSelectionModel* selectionModel;
};

CalendarSearch::Private::Private( CalendarSearch* qq )
  : q( qq )
  , interface( new OrgFreedesktopAkonadiCalendarSearchAgentInterface( QLatin1String("org.freedesktop.Akonadi.Agent.akonadi_calendarsearch_agent"),
                 QLatin1String("/CalendarSearchAgent"),
                 QDBusConnection::sessionBus(),
                 q ) )
  , error( CalendarSearch::NoError )
  , selectionProxyModel( 0 )
  , selectionModel( 0 ) {
    updateTimer.setSingleShot( true );
    updateTimer.setInterval( 0 );
    q->connect( &updateTimer, SIGNAL(timeout()), q, SLOT(updateSearch()) );

    Session *session = new Session( "CalendarSearch-" + KRandom::randomString( 8 ).toLatin1(), q );
    ChangeRecorder *monitor = new ChangeRecorder( q );

    ItemFetchScope scope;
    scope.fetchFullPayload( true );
    scope.fetchAttribute<EntityDisplayAttribute>();

    monitor->setCollectionMonitored( Collection::root() );
    monitor->fetchCollection( true );
    monitor->setItemFetchScope( scope );
    monitor->setMimeTypeMonitored( QLatin1String("text/calendar"), true ); // FIXME: this one should not be needed, in fact it might cause the inclusion of free/busy, notes or other unwanted stuff
    monitor->setMimeTypeMonitored( KCalMimeTypeVisitor::eventMimeType(), true );
    monitor->setMimeTypeMonitored( KCalMimeTypeVisitor::todoMimeType(), true );
    monitor->setMimeTypeMonitored( KCalMimeTypeVisitor::journalMimeType(), true );
    calendarModel = new CalendarModel( session, monitor, q );

    selectionModel = new QItemSelectionModel( calendarModel );
    selectionProxyModel = new KSelectionProxyModel( selectionModel, q );
    selectionProxyModel->setFilterBehavior( KSelectionProxyModel::ChildrenOfExactSelection );
    selectionProxyModel->setSourceModel( calendarModel );

    filterProxy = new EntityMimeTypeFilterModel( q );
    filterProxy->setHeaderGroup( EntityTreeModel::ItemListHeaders );
    filterProxy->setSortRole( CalendarModel::SortRole );
    filterProxy->setSourceModel( selectionProxyModel );
    filterProxy->setDynamicSortFilter( true );

    dateRangeProxyModel = new DateRangeFilterProxyModel;
    dateRangeProxyModel->setDynamicSortFilter( true );
    dateRangeProxyModel->setSourceModel( filterProxy );
}

void CalendarSearch::Private::updateSearch() {
#if 0
    const QString startStr = dateToString( startDate );
    const QString endStr = dateToString( endDate );
    if ( !interface->callWithCallback( QLatin1String("createSearch"), QList<QVariant>() << startStr << endStr, q, SLOT(searchCreated(QVariantMap)) ) ) {
        error = CalendarSearch::SomeError;
        errorString = i18n( "Could not create search." );
        emit q->errorOccurredceived();
    }
#endif
}

void CalendarSearch::Private::triggerDelayedUpdate() {
    if ( !updateTimer.isActive() )
        updateTimer.start();
}

#if 0
void CalendarSearch::Private::searchCreated( const QVariantMap& result ) {
    kDebug() << "search created";
    bool ok = false;
    const int errorCode = result.value( QLatin1String("error") ).toInt();
    if ( errorCode ) {
        error = CalendarSearch::SomeError;
        errorString = result.value( QLatin1String("errorString") ).toString();
        emit q->errorOccurredceived();
        return;
    }
    const int id = result.value( QLatin1String("Collection") ).toInt( &ok );
    if ( !ok || id < 0 ) {
        error = CalendarSearch::SomeError;
        errorString = i18n("Could not parsed the collection ID");
        emit q->errorOccurredceived();
        return;
    }

    Collection col;
    col.setId( id );
    CollectionFetchJob* job = new CollectionFetchJob( col );
    connect( job, SIGNAL(finished(KJob*)), q, SLOT(collectionFetched(KJob*)) );
}

void CalendarSearch::Private::collectionFetched( KJob* j ) {
    const CollectionFetchJob* const job = qobject_cast<const CollectionFetchJob* const>( j );
    Q_ASSERT( job );
    if ( job->error() ) {
        error = CalendarSearch::SomeError;
        errorString = job->errorString();
        emit q->errorOccurredceived();
        return;
    }
    if ( !job->collections().isEmpty() )
      createdCollection = job->collections().first();
}

#endif

CalendarSearch::CalendarSearch( QObject* parent ) : QObject( parent ), d( new Private( this ) ) {
}

QAbstractItemModel* CalendarSearch::model() const {
    return d->dateRangeProxyModel;
}

CalendarSearch::~CalendarSearch() {
    delete d;
}

bool CalendarSearch::hasError() const {
    return d->error != NoError;
}

CalendarSearch::Error CalendarSearch::error() const {
    return d->error;
}

QString CalendarSearch::errorString() const {
    return d->errorString;
}

KDateTime CalendarSearch::startDate() const {
    return d->startDate;
}

void CalendarSearch::setStartDate( const KDateTime& startDate ) {
    if ( d->startDate == startDate )
        return;
    d->startDate = startDate;
    d->dateRangeProxyModel->setStartDate( startDate );
//    d->triggerDelayedUpdate();
}

KDateTime CalendarSearch::endDate() const {
    return d->endDate;
}

void CalendarSearch::setEndDate( const KDateTime& endDate ) {
    if ( d->endDate == endDate )
        return;
    d->endDate = endDate;
    d->dateRangeProxyModel->setEndDate( endDate );
//    d->triggerDelayedUpdate();
}

static QModelIndex findIndex( QAbstractItemModel* m, const QModelIndex& parent, const Collection& c ) {
    const int rows = m->rowCount( parent );
    for ( int i=0; i < rows; ++i ) {
      const QModelIndex idx = m->index( i, 0, parent );
      const Collection found = Akonadi::collectionFromIndex( idx );
      if ( found == c )
        return idx;
      if ( found.isValid() ) {
        const QModelIndex inChildren = findIndex( m, idx, c );
        if ( inChildren.isValid() )
          return inChildren;
      }
    }
    return QModelIndex();
}

void CalendarSearch::Private::collectionSelectionChanged( const QItemSelection& newSelection, const QItemSelection& oldSelection ) {
    kDebug();
    QSet<QModelIndex> oldIndexes = oldSelection.indexes().toSet();
    QSet<QModelIndex> newIndexes = newSelection.indexes().toSet();
    Q_FOREACH( const QModelIndex& i, oldIndexes - newIndexes ) {
        const QModelIndex idx = findIndex( calendarModel, QModelIndex(), Akonadi::collectionFromIndex( i ) );
        if ( idx.isValid() )
            selectionModel->select( idx, QItemSelectionModel::Deselect );
    }
    Q_FOREACH( const QModelIndex& i, newIndexes ) {
        const QModelIndex idx = findIndex( calendarModel, QModelIndex(), Akonadi::collectionFromIndex( i ) );
        if ( idx.isValid() )
            selectionModel->select( idx, QItemSelectionModel::Select );
    }
}

QItemSelectionModel* CalendarSearch::selectionModel() const {
    return d->selectionModel;
}

void CalendarSearch::setSelectionModel( QItemSelectionModel* selectionModel ) {
    connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(collectionSelectionChanged(QItemSelection,QItemSelection)) );
}

#include "calendarsearch.moc"
