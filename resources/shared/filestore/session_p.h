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

#ifndef AKONADI_FILESTORE_SESSION_P_H
#define AKONADI_FILESTORE_SESSION_P_H

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QObject>

namespace Akonadi
{

namespace FileStore
{

class Job;

/**
 */
class AbstractJobSession : public QObject
{
  Q_OBJECT

  public:
    explicit AbstractJobSession( QObject *parent = 0 );

    virtual ~AbstractJobSession();

    virtual void addJob( Job *job ) = 0;

    virtual void cancelAllJobs() = 0;

    void notifyCollectionsReceived( Job *job, const Collection::List &collections );

    void notifyCollectionCreated( Job *job, const Collection &collection );

    void notifyCollectionDeleted( Job *job, const Collection &collection );

    void notifyCollectionModified( Job *job, const Collection &collection );

    void notifyCollectionMoved( Job *job, const Collection &collection );

    void notifyItemsReceived( Job *job, const Item::List &items );

    void notifyItemCreated( Job *job, const Item &item );

    void notifyItemModified( Job *job, const Item &item );

    void notifyItemMoved( Job *job, const Item &item );

    void notifyCollectionsChanged( Job *job, const Collection::List &collections );

    void notifyItemsChanged( Job *job, const Item::List &items );

    void setError( Job *job, int errorCode, const QString& errorText );

    void emitResult( Job *job );

  Q_SIGNALS:
    void jobsReady( const QList<Job*> &jobs );

  protected:
    virtual void removeJob( Job *job ) = 0;
};

}
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
