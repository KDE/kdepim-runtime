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

#ifndef DUMMYRESOURCESTATE_H
#define DUMMYRESOURCESTATE_H

#include <QtCore/QPair>
#include <QtCore/QVariant>

#include "resourcestateinterface.h"

class DummyResourceState : public ResourceStateInterface
{
public:
  typedef boost::shared_ptr<DummyResourceState> Ptr;

  explicit DummyResourceState();
  ~DummyResourceState();

  void setAutomaticExpungeEnagled( bool enabled );
  virtual bool isAutomaticExpungeEnabled() const;

  void setCollection( const Akonadi::Collection &collection );
  virtual Akonadi::Collection collection() const;
  void setItem( const Akonadi::Item &item );
  virtual Akonadi::Item item() const;

  void setParentCollection( const Akonadi::Collection &collection );
  virtual Akonadi::Collection parentCollection() const;

  void setSourceCollection( const Akonadi::Collection &collection );
  virtual Akonadi::Collection sourceCollection() const;
  void setTargetCollection( const Akonadi::Collection &collection );
  virtual Akonadi::Collection targetCollection() const;

  void setParts( const QSet<QByteArray> &parts );
  virtual QSet<QByteArray> parts() const;

  virtual QString rootRemoteId() const;
  virtual QString mailBoxForCollection( const Akonadi::Collection &collection ) const;

  virtual void applyCollectionChanges( const Akonadi::Collection &collection );

  virtual void itemRetrieved( const Akonadi::Item &item );

  virtual void itemsRetrieved( const Akonadi::Item::List &items );
  virtual void itemsRetrievalDone();

  virtual void cancelTask( const QString &errorString );
  virtual void deferTask();

  QList< QPair<QByteArray, QVariant> > calls() const;

private:
  void recordCall( const QByteArray callName, const QVariant &parameter = QVariant() );

  bool m_automaticExpunge;

  Akonadi::Collection m_collection;
  Akonadi::Item m_item;

  Akonadi::Collection m_parentCollection;

  Akonadi::Collection m_sourceCollection;
  Akonadi::Collection m_targetCollection;

  QSet<QByteArray> m_parts;

  QList< QPair<QByteArray, QVariant> > m_calls;
};

#endif
