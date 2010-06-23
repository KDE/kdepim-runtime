/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "resourcestate.h"

#include "imapresource.h"
#include "settings.h"

ResourceStateInterface::Ptr ResourceState::createRetrieveItemState( ImapResource *resource,
                                                                    const Akonadi::Item &item,
                                                                    const QSet<QByteArray> &parts )
{
  ResourceState *state = new ResourceState( resource );

  state->m_item = item;
  state->m_parts = parts;

  return ResourceStateInterface::Ptr( state );
}

ResourceState::ResourceState( ImapResource *resource )
  : m_resource( resource )
{

}

ResourceState::~ResourceState()
{

}

Akonadi::Collection ResourceState::collection() const
{
  return m_collection;
}

Akonadi::Item ResourceState::item() const
{
  return m_item;
}

Akonadi::Collection ResourceState::parentCollection() const
{
  return m_parentCollection;
}

Akonadi::Collection ResourceState::sourceCollection() const
{
  return m_sourceCollection;
}

Akonadi::Collection ResourceState::targetCollection() const
{
  return m_targetCollection;
}

QSet<QByteArray> ResourceState::parts() const
{
  return m_parts;
}

QString ResourceState::rootRemoteId() const
{
  return "imap://" + Settings::self()->userName() + '@' + Settings::self()->imapServer() + '/';
}

QString ResourceState::mailBoxForCollection( const Akonadi::Collection &collection ) const
{
  if ( collection.remoteId().isEmpty() ) {
    kWarning() << "Got incomplete ancestor chain:" << collection;
    return QString();
  }

  if ( collection.parentCollection() == Akonadi::Collection::root() ) {
    kWarning( collection.remoteId() != rootRemoteId() ) << "RID mismatch, is " << collection.remoteId() << " expected " << rootRemoteId();
    return QString( "" );
  }
  const QString parentMailbox = mailBoxForCollection( collection.parentCollection() );
  if ( parentMailbox.isNull() ) // invalid, != isEmpty() here!
    return QString();

  const QString mailbox =  parentMailbox + collection.remoteId();
  if ( parentMailbox.isEmpty() )
    return mailbox.mid( 1 ); // strip of the separator on top-level mailboxes
  return mailbox;
}

void ResourceState::itemRetrieved( const Akonadi::Item &item )
{
  m_resource->itemRetrieved( item );
}

void ResourceState::cancelTask( const QString &errorString )
{
  m_resource->cancelTask( errorString );
}

void ResourceState::deferTask()
{
  m_resource->deferTask();
}

#include "resourcestate.moc"


