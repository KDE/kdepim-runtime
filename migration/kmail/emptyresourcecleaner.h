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

#ifndef EMPTYRESOURCECLEANER_H
#define EMPTYRESOURCECLEANER_H

#include <QObject>

namespace Akonadi
{
  class AgentInstance;
  class Collection;
}

class KJob;

template <typename T> class QList;

class EmptyResourceCleaner : public QObject
{
  Q_OBJECT

  public:
    enum CleaningOption
    {
      CheckOnly = 0x00,
      DeleteEmptyCollections = 0x01,
      DeleteEmptyResource = 0x02
    };

    Q_DECLARE_FLAGS( CleaningOptions, CleaningOption )

    explicit EmptyResourceCleaner( const Akonadi::AgentInstance &resource, QObject *parent = 0 );
    ~EmptyResourceCleaner();

    void setCleaningOptions( CleaningOptions options );

    CleaningOptions cleaningOptions() const;

    QList<Akonadi::Collection> deletableCollections() const;

    bool isResourceDeletable() const;

  Q_SIGNALS:
    void cleanupFinished( const Akonadi::AgentInstance &resource );

  private:
    class Private;
    Private *const d;

  Q_PRIVATE_SLOT( d, void collectionFetchResult( KJob* ) )
  Q_PRIVATE_SLOT( d, void collectionDeleteResult( KJob* ) )
};

Q_DECLARE_OPERATORS_FOR_FLAGS( EmptyResourceCleaner::CleaningOptions )

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
