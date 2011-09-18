/*  This file is part of the KDE project
    Copyright (c) 2011 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef MIXEDMAILDIR_RETRIEVEITEMSJOB_H
#define MIXEDMAILDIR_RETRIEVEITEMSJOB_H

#include <Akonadi/Item>
#include <Akonadi/Job>

namespace Akonadi
{
  class Collection;
  class TransactionSequence;
}

class MixedMaildirStore;

/**
 * Used to implement ResourceBase::retrieveItems() for MixedMail Resource.
 * This completely bypasses ItemSync in order to achieve maximum performance.
 */
class RetrieveItemsJob : public Akonadi::Job
{
  Q_OBJECT
  public:
    RetrieveItemsJob( const Akonadi::Collection &collection, MixedMaildirStore *store, QObject* parent = 0 );
    
    ~RetrieveItemsJob();

    Akonadi::Collection collection() const;
    
    Akonadi::Item::List itemsMarkedAsDeleted() const;
    
  protected:
    void doStart();

  private:
    class Private;
    Private *const d;
      
  Q_PRIVATE_SLOT( d, void akonadiFetchResult( KJob* ) )  
  Q_PRIVATE_SLOT( d, void transactionResult( KJob* ) )
  Q_PRIVATE_SLOT( d, void storeListResult( KJob* ) )
  Q_PRIVATE_SLOT( d, void processNewItem() )
  Q_PRIVATE_SLOT( d, void fetchNewResult( KJob* ) )
  Q_PRIVATE_SLOT( d, void processChangedItem() )
  Q_PRIVATE_SLOT( d, void fetchChangedResult( KJob* ) )
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
