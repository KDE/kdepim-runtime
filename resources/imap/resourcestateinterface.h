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

#ifndef RESOURCESTATEINTERFACE_H
#define RESOURCESTATEINTERFACE_H

#include <Akonadi/Collection>
#include <Akonadi/Item>

#include <boost/shared_ptr.hpp>

class ResourceStateInterface
{
public:
  typedef boost::shared_ptr<ResourceStateInterface> Ptr;

  virtual ~ResourceStateInterface();

  virtual Akonadi::Collection collection() const = 0;
  virtual Akonadi::Item item() const = 0;

  virtual Akonadi::Collection parentCollection() const = 0;

  virtual Akonadi::Collection sourceCollection() const = 0;
  virtual Akonadi::Collection targetCollection() const = 0;

  virtual QSet<QByteArray> parts() const = 0;

  virtual QString rootRemoteId() const = 0;
  virtual QString mailBoxForCollection( const Akonadi::Collection &collection ) const = 0;

  virtual void itemRetrieved( const Akonadi::Item &item ) = 0;
  virtual void cancelTask( const QString &errorString ) = 0;
  virtual void deferTask() = 0;
};

#endif
