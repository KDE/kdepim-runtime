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

#include <kmetadata/kmetadata.h>

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

  Nepomuk::KMetaData::ResourceManager::instance()->setAutoSync( true );

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
  Nepomuk::KMetaData::Resource r( QLatin1String("akonadi://") + QString::number( ref.id() ) );
  r.remove();
}

void MessageSearchProvider::itemChanged(const Akonadi::Item & item)
{
  if ( !item.hasPayload<MessagePtr>() )
    return;
  MessagePtr msg = item.payload<MessagePtr>();
  Nepomuk::KMetaData::Resource r( QLatin1String("akonadi://") + QString::number( item.reference().id() ) );
  if ( msg->subject( false ) )
    r.setProperty( "Subject", msg->subject()->asUnicodeString() );
  if ( msg->date( false ) )
    r.setProperty( "Date", msg->date()->dateTime().dateTime() );
  if ( msg->from( false ) )
    r.setProperty( "From", msg->from()->prettyAddresses() );
  if ( msg->to( false ) )
    r.setProperty( "To", msg->to()->prettyAddresses() );
  if ( msg->cc( false ) )
    r.setProperty( "Cc", msg->cc()->prettyAddresses() );
  if ( msg->bcc( false ) )
    r.setProperty( "Bcc", msg->bcc()->prettyAddresses() );
  if ( msg->messageID( false ) )
    r.setProperty( "Message-Id", msg->messageID()->asUnicodeString() );
}

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );
  Akonadi::SearchProviderBase::init<Akonadi::MessageSearchProvider>( argc, argv, QLatin1String("akonadi_message_searchprovider") );
  return app.exec();
}

#include "messagesearchprovider.moc"
