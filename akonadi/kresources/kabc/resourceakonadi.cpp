// -*- c-basic-offset: 2 -*-
/*
    This file is part of libkabc.
    Copyright (c) 2008-2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#include "resourceakonadi.h"

#include "resourceakonadi_p.h"
#include "resourceakonadiconfig.h"

using namespace KABC;

ResourceAkonadi::ResourceAkonadi()
  : ResourceABC(), d( new Private( this ) )
{
}

ResourceAkonadi::ResourceAkonadi( const KConfigGroup &group )
  : ResourceABC( group ), d( new Private( group, this ) )
{
}

ResourceAkonadi::~ResourceAkonadi()
{
  delete d;
}

void ResourceAkonadi::clear()
{
  // clear local caches
  d->clear();

  ResourceABC::clear();
}

void ResourceAkonadi::writeConfig( KConfigGroup &group )
{
  kDebug(5700);
  ResourceABC::writeConfig( group );

  d->writeConfig( group );
}

bool ResourceAkonadi::doOpen()
{
  return d->doOpen();
}

void ResourceAkonadi::doClose()
{
  d->clear();
  d->doClose();
}

Ticket *ResourceAkonadi::requestSaveTicket()
{
  kDebug(5700);
  if ( !addressBook() ) {
    kDebug(5700) << "no addressbook";
    return 0;
  }

  return createTicket( this );
}

void ResourceAkonadi::releaseSaveTicket( Ticket *ticket )
{
  delete ticket;
}

bool ResourceAkonadi::load()
{
  kDebug(5700);

  d->clear();
  return d->doLoad();
}

bool ResourceAkonadi::asyncLoad()
{
  kDebug(5700);

  d->clear();
  return d->doAsyncLoad();
}

bool ResourceAkonadi::save( Ticket *ticket )
{
  Q_UNUSED( ticket );
  kDebug(5700);

  return d->doSave();
}

bool ResourceAkonadi::asyncSave( Ticket *ticket )
{
  Q_UNUSED( ticket );
  kDebug(5700);

  return d->doAsyncSave();
}

void ResourceAkonadi::insertAddressee( const Addressee &addr )
{
  kDebug(5700);
  if ( d->insertAddressee( addr ) ) {
    ResourceABC::insertAddressee( addr );
  }
}

void ResourceAkonadi::removeAddressee( const Addressee &addr )
{
  kDebug(5700);
  d->removeAddressee( addr );

  ResourceABC::removeAddressee( addr );
}

void ResourceAkonadi::insertDistributionList( DistributionList *list )
{
  kDebug(5700) << "identifier=" << list->identifier()
               << ", name=" << list->name();

  if ( d->insertDistributionList( list ) ) {
    ResourceABC::insertDistributionList( list );
  }
}

void ResourceAkonadi::removeDistributionList( DistributionList *list )
{
  kDebug(5700) << "identifier=" << list->identifier()
               << ", name=" << list->name();

  d->removeDistributionList( list );

  ResourceABC::removeDistributionList( list );
}

bool ResourceAkonadi::subresourceActive( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  bool active = false;
  SubResource *resource = d->subResource( subResource );
  if ( resource != 0 ) {
    active = resource->isActive();
  }

  return active;
}

bool ResourceAkonadi::subresourceWritable( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  bool writable = false;
  SubResource *resource = d->subResource( subResource );
  if ( resource != 0 ) {
    writable = resource->isWritable();
  }

  return writable;
}

QString ResourceAkonadi::subresourceLabel( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  QString label;
  SubResource *resource = d->subResource( subResource );
  if ( resource != 0 ) {
    label = resource->label();
  }

  return label;
}

int ResourceAkonadi::subresourceCompletionWeight( const QString &subResource ) const
{
  kDebug(5700) << "subResource" << subResource;

  int weight = 80;
  SubResource *resource = d->subResource( subResource );
  if ( resource != 0 ) {
    weight = resource->completionWeight();
  }

  return weight;
}

QStringList ResourceAkonadi::subresources() const
{
  kDebug(5700) << d->subResourceIdentifiers();
  return d->subResourceIdentifiers();
}

QMap<QString, QString> ResourceAkonadi::uidToResourceMap() const
{
  return d->uidToResourceMap();
}

StoreConfigIface &ResourceAkonadi::storeConfig()
{
  return *d;
}

void ResourceAkonadi::setSubresourceActive( const QString &subResource, bool active )
{
  kDebug(5700) << "subResource" << subResource << ", active" << active;

  // TODO check if this check for change isn't already handled in the private
  bool changed = false;

  SubResource *resource = d->subResource( subResource );
  if ( resource != 0 ) {
    if ( active != resource->isActive() ) {
      resource->setActive( active );
      changed = true;
    }
  }

  if ( changed )
    addressBook()->emitAddressBookChanged();
}

void ResourceAkonadi::setSubresourceCompletionWeight( const QString &subResource, int weight )
{
  kDebug(5700) << "subResource" << subResource << ", weight" << weight;

  SubResource *resource = d->subResource( subResource );
  if ( resource != 0 ) {
    resource->setCompletionWeight( weight );
  }
}

#include "resourceakonadi.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
