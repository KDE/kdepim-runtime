/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef STORERESULTJOB_H
#define STORERESULTJOB_H

#include <Akonadi/TransactionSequence>

#include <QtCore/QString>

namespace Akonadi {
class Item;
}

/**
 * This class stores the result of a StoreResultJob in an item.
 * First, it removes the 'queued' flag.
 * After that, if the result was success, it stores the 'sent' flag.
 * If the result was failure, it stores the 'error' flag and an ErrorAttribute.
 */
class StoreResultJob : public Akonadi::TransactionSequence
{
  Q_OBJECT

  public:
    /**
     * Creates a new store result job.
     *
     * @param item The item to store.
     * @param success Whether the mail could be dispatched or not.
     * @param message An error message in case the mail could not be dispatched.
     * @param parent The parent object.
     */
    explicit StoreResultJob( const Akonadi::Item &item, bool success, const QString &message, QObject *parent = 0 );

    /**
     * Destroys the store result job.
     */
    virtual ~StoreResultJob();

  protected:
    // reimpl from TransactionSequence
    virtual void doStart();

  private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void fetchDone( KJob *job ) )
    Q_PRIVATE_SLOT( d, void modifyDone( KJob *job ) )
    //@endcond
};

#endif
