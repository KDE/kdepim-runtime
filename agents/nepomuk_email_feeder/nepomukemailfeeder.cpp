/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "nepomukemailfeeder.h"

#include <akonadi/changerecorder.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <nepomuk/resource.h>
#include <nepomuk/variant.h>
#include <kurl.h>

using namespace Akonadi;
typedef boost::shared_ptr<KMime::Message> MessagePtr;

Akonadi::NepomukEMailFeeder::NepomukEMailFeeder( const QString &id ) :
  AgentBase( id )
{
  monitor()->addFetchPart( Item::PartEnvelope );
  monitor()->monitorMimeType( "message/rfc822" );
  monitor()->monitorMimeType( "message/news" );
  monitor()->setChangeRecordingEnabled( false );
}

void NepomukEMailFeeder::itemAdded(const Akonadi::Item & item, const Akonadi::Collection & collection)
{
  Q_UNUSED( collection );
  itemChanged( item, QStringList() );
}

void NepomukEMailFeeder::itemChanged(const Akonadi::Item & item, const QStringList & partIdentifiers)
{
  Q_UNUSED( partIdentifiers );
  if ( !item.hasPayload<MessagePtr>() )
    return;
  MessagePtr msg = item.payload<MessagePtr>();
  Nepomuk::Resource r( item.url().url() );
  if ( msg->subject( false ) )
    r.setProperty( "Subject", Nepomuk::Variant(msg->subject()->asUnicodeString()) );
  if ( msg->date( false ) )
    r.setProperty( "Date", Nepomuk::Variant(msg->date()->dateTime().dateTime()) );
  if ( msg->from( false ) )
    r.setProperty( "From", Nepomuk::Variant(msg->from()->prettyAddresses()) );
  if ( msg->to( false ) )
    r.setProperty( "To", Nepomuk::Variant( msg->to()->prettyAddresses()) );
  if ( msg->cc( false ) )
    r.setProperty( "Cc", Nepomuk::Variant(msg->cc()->prettyAddresses()) );
  if ( msg->bcc( false ) )
    r.setProperty( "Bcc", Nepomuk::Variant(msg->bcc()->prettyAddresses()) );
  if ( msg->messageID( false ) )
    r.setProperty( "Message-Id", Nepomuk::Variant(msg->messageID()->asUnicodeString()) );
}

void NepomukEMailFeeder::itemRemoved(const Akonadi::Item & item)
{
  Nepomuk::Resource r( item.url().url() );
  r.remove();
}

int main( int argc, char **argv )
{
  return AgentBase::init<NepomukEMailFeeder>( argc, argv );
}

#include "nepomukemailfeeder.moc"
