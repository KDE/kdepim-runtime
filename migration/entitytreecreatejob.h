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


#ifndef ENTITYTREECREATEJOB_H
#define ENTITYTREECREATEJOB_H

#include <Akonadi/Job>
#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/TransactionSequence>

class EntityTreeCreateJob : public Akonadi::TransactionSequence
{
  Q_OBJECT
public:
  EntityTreeCreateJob( QList<Akonadi::Collection::List> collections, Akonadi::Item::List items, QObject* parent = 0 );

  /* reimp */ void doStart();

private slots:
  void collectionCreateJobDone( KJob * );

private:
  void createNextLevelOfCollections();
  void createReadyItems();

private:
  QList<Akonadi::Collection::List> m_collections;
  Akonadi::Item::List m_items;
};

#endif
