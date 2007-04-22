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

#include "strigiprovider.h"

#include <libakonadi/itemfetchjob.h>
#include <libakonadi/session.h>
#include <libakonadi/monitor.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

using namespace Akonadi;

Akonadi::StrigiProvider::StrigiProvider(const QString & id) :
    SearchProviderBase( id ),
    mMonitor( 0 )
{
  mMonitor = new Monitor( this );
  mMonitor->monitorAll();
  mMonitor->fetchItemData( true );
  connect( mMonitor, SIGNAL(itemAdded(const Akonadi::Item&)), SLOT(itemChanged(const Akonadi::Item&)) );
  connect( mMonitor, SIGNAL(itemChanged(const Akonadi::Item&, const QStringList&)), SLOT(itemChanged(const Akonadi::Item&)) );
  connect( mMonitor, SIGNAL(itemRemoved(const Akonadi::DataReference&)), SLOT(itemRemoved(const Akonadi::DataReference&)) );
}

Akonadi::StrigiProvider::~ StrigiProvider()
{
  delete mMonitor;
}

void Akonadi::StrigiProvider::itemChanged(const Akonadi::Item & item)
{
  mStrigi.indexFile( "akonadi:/" + QString::number( item.reference().persistanceID() ),
                     QDateTime::currentDateTime().toTime_t(), item.data() );
}

void Akonadi::StrigiProvider::itemRemoved(const Akonadi::DataReference & ref)
{
  mStrigi.indexFile( "akonadi:/" + QString::number( ref.persistanceID() ), QDateTime::currentDateTime().toTime_t(), QByteArray() );
}

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );
  Akonadi::SearchProviderBase::init<Akonadi::StrigiProvider>( argc, argv, QLatin1String("akonadi_strigi_searchprovider") );
  return app.exec();
}

#include "strigiprovider.moc"
