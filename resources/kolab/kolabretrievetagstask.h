/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef KOLABRETRIEVETAGSTASK_H
#define KOLABRETRIEVETAGSTASK_H

#include "kolabrelationresourcetask.h"
#include <akonadi/tag.h>

namespace Kolab
{
    class KolabObjectReader;
} // namespace Kolab

class KolabRetrieveTagTask : public KolabRelationResourceTask
{
    Q_OBJECT
public:
    enum RetrieveType {
        RetrieveTags,
        RetrieveRelations
    };

    explicit KolabRetrieveTagTask(ResourceStateInterface::Ptr resource, RetrieveType type, QObject *parent = 0);

protected:
    virtual void startRelationTask(KIMAP::Session *session);

private:
    KIMAP::Session *mSession;
    Akonadi::Tag::List mTags;
    QHash<QString, Akonadi::Item::List> mTagMembers;
    Akonadi::Relation::List mRelations;
    RetrieveType mRetrieveType;

private Q_SLOTS:
    // void onItemsFetchDone(KJob *job);
    void onFinalSelectDone(KJob *job);
    void onHeadersReceived(const QString &mailBox,
                            const QMap<qint64, qint64> &uids,
                            const QMap<qint64, qint64> &sizes,
                            const QMap<qint64, KIMAP::MessageAttribute> &attrs,
                            const QMap<qint64, KIMAP::MessageFlags> &flags,
                            const QMap<qint64, KIMAP::MessagePtr> &messages);
    void onHeadersFetchDone(KJob *job);

private:
    void extractTag(const Kolab::KolabObjectReader &reader, qint64 remoteUid);
    void extractRelation(const Kolab::KolabObjectReader &reader, qint64 remoteUid);

    // void onApplyCollectionChanged(const Akonadi::Collection &collection);
    // void onCancelTask(const QString &errorText);
    // void onChangeCommitted();
};

#endif // KOLABCHANGETAGTASK_H
