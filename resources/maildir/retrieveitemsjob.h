/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef MAILDIR_RETRIEVEITEMSJOB_H
#define MAILDIR_RETRIEVEITEMSJOB_H

#include <collection.h>
#include <item.h>
#include <job.h>

#include "maildir.h"

class QDirIterator;
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
    RetrieveItemsJob(const Akonadi::Collection &collection, const KPIM::Maildir &md, QObject *parent = nullptr);
    void setMimeType(const QString &mimeType);

protected:
    void doStart() override;

private:
    void entriesProcessed();
    Akonadi::TransactionSequence *transaction();

private Q_SLOTS:
    void localListDone(KJob *job);
    void transactionDone(KJob *job);
    void processEntry();
    void processEntryDone(KJob *);

private:
    Akonadi::Collection m_collection;
    KPIM::Maildir m_maildir;
    QHash<QString, Akonadi::Item> m_localItems;
    QString m_mimeType;
    Akonadi::TransactionSequence *m_transaction = nullptr;
    int m_transactionSize;
    QDirIterator *m_entryIterator = nullptr;
    qint64 m_previousMtime;
    qint64 m_highestMtime;
    QString m_listingPath;
};

#endif
