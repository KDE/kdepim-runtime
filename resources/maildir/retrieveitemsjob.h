/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#ifndef MAILDIR_RETRIEVEITEMSJOB_H
#define MAILDIR_RETRIEVEITEMSJOB_H

#include <Akonadi/Item>
#include <Akonadi/Job>
#include <Akonadi/Collection>

#include "maildir.h"

namespace Akonadi
{
class TransactionSequence;
}

/**
 * Used to implement ResourceBase::retrieveItems() for Maildirs.
 * This completely bypasses ItemSync in order to achieve maximum performance.
 */
class RetrieveItemsJob : public Akonadi::Job
{
  Q_OBJECT
  public:
    RetrieveItemsJob( const Akonadi::Collection &collection, const KPIM::Maildir &md, QObject* parent = 0 );
    void setMimeType( const QString &mimeType );

  protected:
    void doStart();

  private:
    void entriesProcessed();
    Akonadi::TransactionSequence* transaction();

  private slots:
    void localListDone( KJob *job );
    void transactionDone( KJob *job );
    void processEntry( qint64 index );

  private:
    Akonadi::Collection m_collection;
    KPIM::Maildir m_maildir;
    QHash<QString, Akonadi::Item> m_localItems;
    QString m_mimeType;
    Akonadi::TransactionSequence *m_transaction;
    QStringList m_entryList;
    qint64 m_previousMtime;
    qint64 m_highestMtime;
    QString m_listingPath;
};

#endif
