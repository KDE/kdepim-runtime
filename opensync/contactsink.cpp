#include "contactsink.h"

#include <kabc/vcardconverter.h>
#include <kabc/addressee.h>

#include <KDebug>

//typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

using namespace Akonadi;

ContactSink::ContactSink() :
  DataSink( GetChanges | Commit | SyncDone )
{
  kDebug();
}

void ContactSink::reportChange( const Item& item )
{
  DataSink::reportChange( item, "vcard30" );
}

int ContactSink::createItem( OSyncData *data )
{
  char *plain = 0;
  osync_data_get_data( data, &plain, /*size*/0 );
  QString str = QString::fromUtf8( plain );

  Collection col = collection();
  if ( !col.isValid() ) // error handling?
    return -1;

  KABC::VCardConverter converter;
  KABC::Addressee vcard = converter.parseVCard( str.toUtf8() );
  kDebug() << vcard.uid() << vcard.name();

  Akonadi::Item contact;
  contact.setMimeType( "text/directory" );
  contact.setPayload<KABC::Addressee>( vcard );

  ItemCreateJob *job = new Akonadi::ItemCreateJob( contact, col );

  if( ! job->exec() )
    kDebug() << "creating a vcard failed";

    return contact.id(); // errorhandling for job->exec() ..
}

int ContactSink::modifyItem( OSyncData *data )
{
  kDebug();
}

#include "contactsink.moc"
