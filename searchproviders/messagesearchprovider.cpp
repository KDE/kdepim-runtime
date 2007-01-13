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
#include "messagesearchproviderthread.h"

#include <libakonadi/itemfetchjob.h>
#include <libakonadi/job.h>
#include <libakonadi/monitor.h>

#include <kmetadata/kmetadata.h>

#include <QtCore/QCoreApplication>

using namespace Akonadi;

Akonadi::MessageSearchProvider::MessageSearchProvider( const QString &id ) :
  SearchProviderBase( id )
{
  Monitor* monitor = new Monitor( this );
  monitor->monitorMimeType( "message/rfc822" );
  monitor->monitorMimeType( "message/news" );
  connect( monitor, SIGNAL(itemAdded(const Akonadi::DataReference&)), SLOT(itemChanged(const Akonadi::DataReference&)) );
  connect( monitor, SIGNAL(itemChanged(const Akonadi::DataReference&)), SLOT(itemChanged(const Akonadi::DataReference&)) );
  connect( monitor, SIGNAL(itemRemoved(const Akonadi::DataReference&)), SLOT(itemRemoved(const Akonadi::DataReference&)) );

  Nepomuk::KMetaData::ResourceManager::instance()->setAutoSync( true );
}

QList< QByteArray > Akonadi::MessageSearchProvider::supportedMimeTypes() const
{
  QList<QByteArray> mimeTypes;
  mimeTypes << "message/rfc822" << "message/news";
  return mimeTypes;
}

SearchProviderThread* Akonadi::MessageSearchProvider::workerThread()
{
  return new MessageSearchProviderThread( this );
}

void MessageSearchProvider::itemChanged(const Akonadi::DataReference & ref)
{
  ItemFetchJob *job = new ItemFetchJob( ref, this );
  connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(itemReceived(Akonadi::Job*)) );
  job->start();
}

void MessageSearchProvider::itemRemoved(const Akonadi::DataReference & ref)
{
  Nepomuk::KMetaData::Resource r( QLatin1String("akonadi://") + QString::number( ref.persistanceID() ) );
  r.remove();
}

void MessageSearchProvider::itemReceived(Akonadi::Job * job)
{
  if ( job->error() || static_cast<ItemFetchJob*>( job )->items().count() == 0 ) {
    // TODO: erro handling
    qDebug() << "Job error:" << job->errorMessage();
  } else {
    Item *item = static_cast<ItemFetchJob*>( job )->items().first();
    Q_ASSERT( item );
    Nepomuk::KMetaData::Resource r( QLatin1String("akonadi://") + QString::number( item->reference().persistanceID() ) );
    // TODO
    r.setProperty( "Subject", "Test" );
  }
  job->deleteLater();
}

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );
  Akonadi::SearchProviderBase::init<Akonadi::MessageSearchProvider>( argc, argv, QLatin1String("akonadi_message_searchprovider") );
  return app.exec();
}

#include "messagesearchprovider.moc"
