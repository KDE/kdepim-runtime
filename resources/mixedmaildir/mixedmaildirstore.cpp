/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

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

#include "mixedmaildirstore.h"

#include <akonadi/kmime/messageparts.h>

#include <akonadi/cachepolicy.h>

#include <QStringList>

using namespace Akonadi;
using namespace Akonadi::FileStore;

class MixedMaildirStore::Private
{
  MixedMaildirStore *const q;

  public:
    Private( MixedMaildirStore *parent ) : q( parent )
    {
    }
};

MixedMaildirStore::MixedMaildirStore() : AbstractLocalStore(), d( new Private( this ) )
{
}

MixedMaildirStore::~MixedMaildirStore()
{
  delete d;
}

void MixedMaildirStore::setTopLevelCollection( const Akonadi::Collection &collection )
{
  QStringList contentMimeTypes;
  contentMimeTypes << Collection::mimeType();
  // TODO if top level can contain mails, add that MIME type as well

  Collection::Rights rights;
  // TODO check if read-only?
  rights = Collection::CanCreateCollection | Collection::CanChangeCollection | Collection::CanDeleteCollection;
  // TODO if top level can contain mails, add item rights as well

  CachePolicy cachePolicy;
  cachePolicy.setInheritFromParent( false );
  cachePolicy.setLocalParts( QStringList() << MessagePart::Header );

  Collection modifiedCollection = collection;
  modifiedCollection.setContentMimeTypes( contentMimeTypes );
  modifiedCollection.setRights( rights );
  modifiedCollection.setParentCollection( Collection::root() );
  modifiedCollection.setCachePolicy( cachePolicy );

  AbstractLocalStore::setTopLevelCollection( modifiedCollection );
}

void MixedMaildirStore::processJob( Job *job )
{
  // TODO
}

#include "mixedmaildirstore.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
