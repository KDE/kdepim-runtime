/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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


#include "akonadiengine.h"

#include <libakonadi/itemfetchjob.h>
#include <libakonadi/monitor.h>
using namespace Akonadi;

AkonadiEngine::AkonadiEngine(QObject* parent, const QStringList& args)
    : Plasma::DataEngine(parent)
{
  Q_UNUSED(args);
  setSourceLimit( 4 );

  Monitor *monitor = new Monitor( this );
  monitor->monitorMimeType( "message/rfc822" );
  connect( monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
           SLOT(itemAdded(Akonadi::Item)) );

  // FIXME: hardcoded collection id
  ItemFetchJob *fetch = new ItemFetchJob( Collection( 421 ), this );
  connect( fetch, SIGNAL(result(KJob*)), SLOT(fetchDone(KJob*)) );
}

AkonadiEngine::~AkonadiEngine()
{
}

void AkonadiEngine::itemAdded(const Akonadi::Item & item)
{
  setData( QString::number( item.reference().id() ), "Subject", "FOO" );
}

void AkonadiEngine::fetchDone(KJob * job)
{
  if ( job->error() )
    return;
  Item::List items = static_cast<ItemFetchJob*>( job )->items();
  foreach ( const Item item, items ) {
    setData( QString::number( item.reference().id() ), "Subject", "BAR" );
  }
}

#include "akonadiengine.moc"
