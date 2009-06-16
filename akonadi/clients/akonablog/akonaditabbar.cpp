/*
    Copyright (C) 2009 Omat Holding B.V. <info@omat.nl>

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

#include "akonaditabbar.h"
#include <kdebug.h>

#include <KTabBar>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>

using namespace Akonadi;

class AkonadiTabBar::Private
{
public:
    Private( AkonadiTabBar* parent );
    QHash<int,Collection> tabs;
    AkonadiTabBar* q;

public slots:
    void slotCurrentChanged( int );
};

AkonadiTabBar::Private::Private( AkonadiTabBar* parent ) : q( parent )
{
}

void AkonadiTabBar::Private::slotCurrentChanged( int index )
{
    q->emit currentChanged( tabs.value( index ) );
}

AkonadiTabBar::AkonadiTabBar( QWidget* parent )
        :  KTabBar( parent ), d( new Private( this ) )
{
    connect( this, SIGNAL( currentChanged( int ) ),
             SLOT( slotCurrentChanged( int ) ) );
}

AkonadiTabBar::~AkonadiTabBar()
{
    delete d;
}

void AkonadiTabBar::setResource( const QString &resource )
{
    // save it for later.
    int pastIndex = currentIndex();

    // remote old tabs.
    while ( count() > 0 )
        removeTab( 0 );
    d->tabs.clear();

    // fetching all collections recursive, starting at the root collection
    Collection col;
    CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
    job->setResource( resource );
    if ( job->exec() ) {
        Collection::List collections = job->collections();
        foreach( const Collection &collection, collections ) {
            if ( collection.parent() != Collection::root().id() ) {
                int tab = addTab( collection.name() );
                d->tabs[ tab ] = collection;
            }
        }
    }

    // setCurrent would not result in the emission of the signal, as that is already the current one.
    if ( pastIndex <= 0 )
        emit currentChanged( d->tabs.value( 0 ) );
    else
        setCurrentIndex( pastIndex );
}


#include "akonaditabbar.moc"
