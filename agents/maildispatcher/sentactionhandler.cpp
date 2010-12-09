/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Tobias Koenig <tokoe@kdab.com>

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

#include "sentactionhandler.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/kmime/messageflags.h>

using namespace MailTransport;

SentActionHandler::SentActionHandler( QObject *parent )
  : QObject( parent )
{
}

void SentActionHandler::runAction( const SentActionAttribute::Action &action )
{
  if ( action.type() == SentActionAttribute::Action::MarkAsReplied ||
       action.type() == SentActionAttribute::Action::MarkAsForwarded ) {

    const Akonadi::Item item( action.value().toLongLong() );
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( itemFetchResult( KJob* ) ) );
    job->setProperty( "type", static_cast<int>( action.type() ) );
  }
}

void SentActionHandler::itemFetchResult( KJob *job )
{
  if ( job->error() ) {
    kWarning() << job->errorText();
    return;
  }

  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
  if ( fetchJob->items().isEmpty() )
    return;

  Akonadi::Item item = fetchJob->items().first();

  const SentActionAttribute::Action::Type type = static_cast<SentActionAttribute::Action::Type>( job->property( "type" ).toInt() );
  if ( type == SentActionAttribute::Action::MarkAsReplied ) {
    item.setFlag( Akonadi::MessageFlags::Replied );
  } else if ( type == SentActionAttribute::Action::MarkAsForwarded ) {
    item.setFlag( Akonadi::MessageFlags::Forwarded );
  }

  Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob( item );
  modifyJob->setIgnorePayload( true );
}

#include "sentactionhandler.moc"
