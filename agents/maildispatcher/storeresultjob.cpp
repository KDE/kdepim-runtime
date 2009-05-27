/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "storeresultjob.h"

#include <KDebug>
#include <KLocalizedString>

#include <Akonadi/Item>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemModifyJob>

#include <outboxinterface/errorattribute.h>

using namespace Akonadi;
using namespace OutboxInterface;


/**
 * @internal
 */
class StoreResultJob::Private
{
  public:
    Private( StoreResultJob *qq )
      : q( qq )
    {
    }

    StoreResultJob *const q;
    Item item;
    bool success;
    QString message;

    // slots:
    void fetchDone( KJob *job );
    void modifyDone( KJob *job );

};


void StoreResultJob::Private::fetchDone( KJob *job )
{
  if( job->error() )
    return;

  kDebug();

  ItemFetchJob *fjob = dynamic_cast<ItemFetchJob*>( job );
  Q_ASSERT( fjob );
  if( fjob->items().count() != 1 ) {
    kError() << "Fetched" << fjob->items().count() << "items, expected 1.";
    q->setError( Unknown );
    q->setErrorText( QLatin1String( "Failed to fetch item." ) );
    q->commit();
    return;
  }
  
  // Store result in item.
  Item item = fjob->items().first();
  item.clearFlag( "queued" );
  if( success ) {
    item.setFlag( "sent" );
  } else {
    item.setFlag( "error" );
    ErrorAttribute *eA = new ErrorAttribute( message );
    item.addAttribute( eA );
  }

  ItemModifyJob *mjob = new ItemModifyJob( item, q );
  QObject::connect( mjob, SIGNAL( result( KJob* ) ), q, SLOT( modifyDone( KJob* ) ) );
}

void StoreResultJob::Private::modifyDone( KJob *job )
{
  if( job->error() )
    return;

  kDebug();

  q->commit();
}



StoreResultJob::StoreResultJob( const Item &item, bool success, const QString &message, QObject *parent )
  : TransactionSequence( parent )
  , d( new Private( this ) )
{
  d->item = item;
  d->success = success;
  d->message = message;
}

StoreResultJob::~StoreResultJob()
{
  delete d;
}

void StoreResultJob::doStart()
{
  // Fetch item in case it was modified elsewhere.
  ItemFetchJob *job = new ItemFetchJob( d->item, this );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( fetchDone( KJob* ) ) );
}


#include "storeresultjob.moc"
