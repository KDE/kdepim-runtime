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

#include "itemmovejob.h"

#include "session_p.h"

using namespace Akonadi::FileStore;

class ItemMoveJob::ItemMoveJob::Private
{
  public:
    explicit Private( ItemMoveJob *parent )
      : mParent( parent )
    {
    }

  public:
    Akonadi::Item mItem;
    Akonadi::Collection mTargetParent;

  private:
    ItemMoveJob *mParent;
};

ItemMoveJob::ItemMoveJob( const Item &item, const Akonadi::Collection &targetParent, AbstractJobSession *session )
  : Job( session ), d( new Private( this ) )
{
  d->mItem = item;
  d->mTargetParent = targetParent;

  session->addJob( this );
}

ItemMoveJob::~ItemMoveJob()
{
  delete d;
}

Akonadi::Collection ItemMoveJob::targetParent() const
{
  return d->mTargetParent;
}

Akonadi::Item ItemMoveJob::item() const
{
  return d->mItem;
}

bool ItemMoveJob::accept( Visitor *visitor )
{
  return visitor->visit( this );
}

void ItemMoveJob::handleItemMoved( const Item &item )
{
  d->mItem = item;
}

#include "itemmovejob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
