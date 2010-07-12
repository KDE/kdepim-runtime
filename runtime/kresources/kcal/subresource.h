/*
    This file is part of libkcal.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef KCAL_RESOURCEAKONADI_SUBRESOURCE_H
#define KCAL_RESOURCEAKONADI_SUBRESOURCE_H

#include "idarbiterbase.h"
#include "subresourcebase.h"

#include <kcal/incidence.h>

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

class IdArbiter : public IdArbiterBase
{
  protected:
    QString createArbitratedId() const;
};

class SubResource : public SubResourceBase
{
  Q_OBJECT

  public:
    explicit SubResource( const Akonadi::Collection &collection );

    ~SubResource();

    static QStringList supportedMimeTypes();

    bool createChildSubResource( const QString &resourceName );

    bool remove();

    QString subResourceType() const;

  Q_SIGNALS:
    void incidenceAdded( const IncidencePtr &incidencePtr, const QString &subResourceIdentifier );

    void incidenceChanged( const IncidencePtr &incidencePtr, const QString &subResourceIdentifier );

    void incidenceRemoved( const QString &uid, const QString &subResourceIdentifier );

  protected:
    void itemAdded( const Akonadi::Item &item );

    void itemChanged( const Akonadi::Item &item );

    void itemRemoved( const Akonadi::Item &item );
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
