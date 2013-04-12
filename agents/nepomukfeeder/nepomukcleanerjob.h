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

#ifndef REMOVEITEMSJOB_H
#define REMOVEITEMSJOB_H
#include <KJob>
#include <QTime>
#include <Akonadi/Item>
#include <Akonadi/Collection>


/**
 * Job that removes the passed in list of stale items from nepomuk
 */
class NepomukCleanerJob: public KJob
{
    Q_OBJECT
public:
    explicit NepomukCleanerJob(const QList<QUrl> &uriList, QObject* parent = 0);
    virtual void start();
private slots:
    void removedItem(KJob*);
private:
    QList<QUrl> mStaleUris;
    void remove();
    int mBatchSize;
};

#endif // FINDUNINDEXEDITEMSJOB_H
