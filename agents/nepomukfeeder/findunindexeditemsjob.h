/*
 *   Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FINDUNINDEXEDITEMSJOB_H
#define FINDUNINDEXEDITEMSJOB_H
#include <KJob>
#include <QTime>
#include <Akonadi/Item>
#include <Akonadi/Collection>

namespace Soprano {
    namespace Util {
        class AsyncQuery;
    }
}

class FindUnindexedItemsJob: public KJob
{
    Q_OBJECT
public:

    typedef QHash< Akonadi::Item::Id, QPair< QDateTime, QString > > ItemHash;

    explicit FindUnindexedItemsJob(int compatLevel, QObject* parent = 0);
    ~FindUnindexedItemsJob();
    virtual void start();
    /// Returns all items which were found in akonadi but not in nepomuk (meaning they should be indexed)
    const ItemHash &getUnindexed() const;

    /**
     * Returns a list of Nepomuk URIs which are Akonadi DataObjects
     * but are no longer present in Akonadi i.e they can now be removed
     */
    const QList<QUrl> &staleUris() const;

    /// Filter the searched items by indexed collections 
    void setIndexedCollections(const Akonadi::Collection::List &);
    int indexedCount() const;
    int totalCount() const;
private slots:
    void jobDone(KJob*);
    void retrieveIndexedNepomukResources();
    void queryFinished(Soprano::Util::AsyncQuery *);
    void processResult(Soprano::Util::AsyncQuery *);
    void itemsReceived(const Akonadi::Item::List &);
private:
    void fetchItemsFromCollection();
    ItemHash mAkonadiItems;

    /// Contain a list of Nepomuk Uri which are now invalid
    QList<QUrl> mStaleUris;
    QTime mTime;
    const int mCompatLevel;
    Akonadi::Collection::List mIndexedCollections;
    int mTotalNumberOfItems;
    QSharedPointer<Soprano::Util::AsyncQuery> mQuery;
};

#endif // FINDUNINDEXEDITEMSJOB_H
