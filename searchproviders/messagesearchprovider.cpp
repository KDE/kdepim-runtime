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

#include "messagesearchprovider.h"

#include <libakonadi/itemfetchjob.h>
#include <libakonadi/job.h>
#include <libakonadi/session.h>
#include <libakonadi/monitor.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <nepomuk/resource.h>
#include <nepomuk/variant.h>
#include <kurl.h>

#include <QtCore/QCoreApplication>

using namespace Akonadi;
typedef boost::shared_ptr<KMime::Message> MessagePtr;

Akonadi::MessageSearchProvider::MessageSearchProvider( const QString &id ) :
  SearchProviderBase( id )
{
  Monitor* monitor = new Monitor( this );
  monitor->addFetchPart( ItemFetchJob::PartAll );
  monitor->monitorMimeType( "message/rfc822" );
  monitor->monitorMimeType( "message/news" );
  connect( monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)), SLOT(itemChanged(Akonadi::Item)) );
  connect( monitor, SIGNAL(itemChanged(const Akonadi::Item&, const QStringList&)), SLOT(itemChanged(const Akonadi::Item&)) );
  connect( monitor, SIGNAL(itemRemoved(const Akonadi::DataReference&)), SLOT(itemRemoved(const Akonadi::DataReference&)) );

  mSession = new Session( id.toLatin1(), this );
  monitor->ignoreSession( mSession );
}

QStringList Akonadi::MessageSearchProvider::supportedMimeTypes() const
{
  QStringList mimeTypes;
  mimeTypes << QString::fromLatin1( "message/rfc822" ) << QString::fromLatin1( "message/news" );
  return mimeTypes;
}

void MessageSearchProvider::itemRemoved(const Akonadi::DataReference & ref)
{
  Nepomuk::Resource r( Item( ref ).url().url() );
  r.remove();
}

void MessageSearchProvider::itemChanged(const Akonadi::Item & item)
{
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

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );
  Akonadi::SearchProviderBase::init<Akonadi::MessageSearchProvider>( argc, argv, QLatin1String("akonadi_message_searchprovider") );
  return app.exec();
}

#include "messagesearchprovider.moc"
