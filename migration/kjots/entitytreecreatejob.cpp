/*
    Copyright (c) 2010 Stephen Kelly <steveire@gmail.com>

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


#include "entitytreecreatejob.h"

#include <Akonadi/CollectionCreateJob>
#include <Akonadi/ItemCreateJob>

using namespace Akonadi;

EntityTreeCreateJob::EntityTreeCreateJob( QList< Akonadi::Collection::List > collections, Akonadi::Item::List items, QObject* parent )
  : Job( parent ), m_collections( collections ), m_items( items )
{

}

void EntityTreeCreateJob::doStart()
{
  foreach (const Collection::List &colList, m_collections)
    foreach (const Collection &collection, colList)
      (void) new CollectionCreateJob(collection, this);

  foreach (const Item &item, m_items)
    (void) new ItemCreateJob(item, item.parentCollection(), this);
}

