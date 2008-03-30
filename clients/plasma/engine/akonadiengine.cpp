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

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>

#include <kmime/kmime_message.h>

#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

AkonadiEngine::AkonadiEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent)
{
  Q_UNUSED(args);
  setSourceLimit( 4 );

  Monitor *monitor = new Monitor( this );
  monitor->monitorMimeType( "message/rfc822" );
  monitor->itemFetchScope().addFetchPart( Item::PartEnvelope );
  connect( monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
           SLOT(itemAdded(Akonadi::Item)) );
  connect( monitor, SIGNAL(itemChanged(Akonadi::Item,QStringList)),
           SLOT(itemChanged(Akonadi::Item)) );

  // FIXME: hardcoded collection id
  ItemFetchJob *fetch = new ItemFetchJob( Collection( 421 ), this );
  fetch->fetchScope().addFetchPart( Item::PartEnvelope );
  connect( fetch, SIGNAL(result(KJob*)), SLOT(fetchDone(KJob*)) );
}

AkonadiEngine::~AkonadiEngine()
{
}

void AkonadiEngine::itemAdded(const Akonadi::Item & item)
{
  if ( !item.hasPayload<MessagePtr>() )
    return;
  MessagePtr msg = item.payload<MessagePtr>();
  const QString source = QString::number( item.id() );
  setData( source, "Subject", msg->subject()->asUnicodeString() );
  setData( source, "From", msg->from()->asUnicodeString() );
}

void AkonadiEngine::fetchDone(KJob * job)
{
  if ( job->error() )
    return;
  Item::List items = static_cast<ItemFetchJob*>( job )->items();
  foreach ( const Item item, items )
    itemAdded( item );
}

void AkonadiEngine::itemChanged(const Akonadi::Item & item)
{
  const QString source = QString::number( item.id() );
  if ( !sources().contains( source ) )
    return;
  MessagePtr msg = item.payload<MessagePtr>();
  setData( source, "Subject", msg->subject()->asUnicodeString() );
  setData( source, "From", msg->from()->asUnicodeString() );
}

#include "akonadiengine.moc"
