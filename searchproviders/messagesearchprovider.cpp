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

#include <libakonadi/messagefetchjob.h>
#include <libakonadi/job.h>
#include <libakonadi/session.h>
#include <libakonadi/monitor.h>

#include <kmime/kmime_message.h>

#include <kmetadata/kmetadata.h>

#include <QtCore/QCoreApplication>

using namespace Akonadi;

Akonadi::MessageSearchProvider::MessageSearchProvider( const QString &id ) :
  SearchProviderBase( id )
{
  Monitor* monitor = new Monitor( this );
  monitor->monitorMimeType( "message/rfc822" );
  monitor->monitorMimeType( "message/news" );
  connect( monitor, SIGNAL(itemAdded(const Akonadi::Item&)), SLOT(itemChanged(const Akonadi::Item&)) );
  connect( monitor, SIGNAL(itemChanged(const Akonadi::Item&)), SLOT(itemChanged(const Akonadi::Item&)) );
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

void MessageSearchProvider::itemChanged(const Akonadi::Item & item)
{
  MessageFetchJob *job = new MessageFetchJob( item.reference(), mSession );
  connect( job, SIGNAL(result(KJob*)), SLOT(itemReceived(KJob*)) );
}

void MessageSearchProvider::itemRemoved(const Akonadi::DataReference & ref)
{
  Nepomuk::KMetaData::Resource r( QLatin1String("akonadi://") + QString::number( ref.persistanceID() ) );
  r.remove();
}

void MessageSearchProvider::itemReceived(KJob * job)
{
  if ( job->error() || static_cast<MessageFetchJob*>( job )->items().count() == 0 ) {
    // TODO: erro handling
    qDebug() << "Job error:" << job->errorString();
  } else {
    Message *msg = static_cast<MessageFetchJob*>( job )->messages().first();
    Q_ASSERT( msg && msg->mime() );
    Nepomuk::KMetaData::EMail r( QLatin1String("akonadi://") + QString::number( msg->reference().persistanceID() ) );
    if ( msg->mime()->subject( false ) )
      r.setProperty( "Subject", msg->mime()->subject()->asUnicodeString() );
    if ( msg->mime()->date( false ) )
      r.setProperty( "Date", msg->mime()->date()->dateTime().dateTime() );
    if ( msg->mime()->from( false ) )
      r.setProperty( "From", msg->mime()->from()->prettyAddresses() );
    if ( msg->mime()->to( false ) )
      r.setProperty( "To", msg->mime()->to()->prettyAddresses() );
    if ( msg->mime()->cc( false ) )
      r.setProperty( "Cc", msg->mime()->cc()->prettyAddresses() );
    if ( msg->mime()->bcc( false ) )
      r.setProperty( "Bcc", msg->mime()->bcc()->prettyAddresses() );
    if ( msg->mime()->messageID( false ) )
      r.setProperty( "Message-Id", msg->mime()->messageID()->asUnicodeString() );
  }
}

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );
  Akonadi::SearchProviderBase::init<Akonadi::MessageSearchProvider>( argc, argv, QLatin1String("akonadi_message_searchprovider") );
  return app.exec();
}

#include "messagesearchprovider.moc"
