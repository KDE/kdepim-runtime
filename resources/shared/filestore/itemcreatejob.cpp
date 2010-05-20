/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "itemcreatejob.h"

#include "session_p.h"

using namespace Akonadi::FileStore;

class ItemCreateJob::ItemCreateJob::Private
{
  public:
    explicit Private( ItemCreateJob *parent )
      : mParent( parent )
    {
    }

  public:
    Akonadi::Item mItem;
    Akonadi::Collection mCollection;

  private:
    ItemCreateJob *mParent;
};

ItemCreateJob::ItemCreateJob( const Akonadi::Item &item, const Akonadi::Collection &collection, AbstractJobSession *session )
  : Job( session ), d( new Private( this ) )
{
  d->mItem = item;
  d->mCollection = collection;

  session->addJob( this );
}

ItemCreateJob::~ItemCreateJob()
{
  delete d;
}

Akonadi::Collection ItemCreateJob::collection() const
{
  return d->mCollection;
}

Akonadi::Item ItemCreateJob::item() const
{
  return d->mItem;
}

bool ItemCreateJob::accept( Visitor *visitor )
{
  return visitor->visit( this );
}

void ItemCreateJob::handleItemCreated( const Akonadi::Item &item )
{
  d->mItem = item;
}

#include "itemcreatejob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
