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

#include "dummyresourcestate.h"

DummyResourceState::DummyResourceState()
{

}

DummyResourceState::~DummyResourceState()
{

}

void DummyResourceState::setCollection( const Akonadi::Collection &collection )
{
  m_collection = collection;
}

Akonadi::Collection DummyResourceState::collection() const
{
  return m_collection;
}

void DummyResourceState::setItem( const Akonadi::Item &item )
{
  m_item = item;
}

Akonadi::Item DummyResourceState::item() const
{
  return m_item;
}

void DummyResourceState::setParentCollection( const Akonadi::Collection &collection )
{
  m_parentCollection = collection;
}

Akonadi::Collection DummyResourceState::parentCollection() const
{
  return m_parentCollection;
}

void DummyResourceState::setSourceCollection( const Akonadi::Collection &collection )
{
  m_sourceCollection = collection;
}

Akonadi::Collection DummyResourceState::sourceCollection() const
{
  return m_sourceCollection;
}

void DummyResourceState::setTargetCollection( const Akonadi::Collection &collection )
{
  m_targetCollection = collection;
}

Akonadi::Collection DummyResourceState::targetCollection() const
{
  return m_targetCollection;
}

void DummyResourceState::setParts( const QSet<QByteArray> &parts )
{
  m_parts = parts;
}

QSet<QByteArray> DummyResourceState::parts() const
{
  return m_parts;
}

QString DummyResourceState::rootRemoteId() const
{
  return QString();
}

QString DummyResourceState::mailBoxForCollection( const Akonadi::Collection &collection ) const
{
  return collection.remoteId();
}

void DummyResourceState::itemRetrieved( const Akonadi::Item &item )
{
  m_calls << QPair<QByteArray, QVariant>( "itemRetrieved", QVariant::fromValue(item) );
}

void DummyResourceState::cancelTask( const QString &errorString )
{
  m_calls << QPair<QByteArray, QVariant>( "cancelTask", QVariant::fromValue(errorString) );
}

void DummyResourceState::deferTask()
{
  m_calls << QPair<QByteArray, QVariant>( "deferTask", QVariant() );
}

QList< QPair<QByteArray, QVariant> > DummyResourceState::calls() const
{
  return m_calls;
}


