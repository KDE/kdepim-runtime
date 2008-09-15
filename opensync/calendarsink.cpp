#include "calendarsink.h"

#include <kcal/incidence.h>
#include <kcal/icalformat.h>

#include <KDebug>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

using namespace Akonadi;

CalendarSink::CalendarSink() :
  DataSink( GetChanges | Commit | SyncDone )
{
kDebug();
}

void CalendarSink::reportChange( const Item& item )
{
kDebug();
  DataSink::reportChange( item, "vevent20" );
}

int CalendarSink::createItem( OSyncData *data )
{
  char *plain = 0;
  osync_data_get_data( data, &plain, /*size*/0 );
  QString str = QString::fromUtf8( plain );

  KCal::ICalFormat format;
  KCal::Incidence *item = format.fromString( str );
  kDebug() << item->uid() << item->summary();

  Collection col = collection();
  if ( !col.isValid() ) // error handling
    return -1;

  Akonadi::Item event;
  event.setMimeType( "application/x-vnd.akonadi.calendar.event" );
  event.setPayload<IncidencePtr>( IncidencePtr( item->clone() ) );
  //event.setRemoteId( item->uid() );

  ItemCreateJob *job = new Akonadi::ItemCreateJob( event, col );

  if( ! job->exec() )
    kDebug() << "creating a cal entry failed";

  return event.revision(); // handle !job->exec in return too..
}

int CalendarSink::modifyItem( OSyncData *data )
{
kDebug();
}

#include "calendarsink.moc"
