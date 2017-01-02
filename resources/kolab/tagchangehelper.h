/*
    Copyright (c) 2014 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Krammer <kevin.krammer@kdab.com>

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

#ifndef TAGCHANGEHELPER_H
#define TAGCHANGEHELPER_H

#include <collection.h>
#include <item.h>
#include <kmime/kmime_message.h>

#include <QObject>

namespace KIMAP
{
class Session;
}

namespace Akonadi
{
class Tag;
}

class KolabRelationResourceTask;

struct TagConverter {
    KMime::Message::Ptr createMessage(const Akonadi::Tag &tag, const Akonadi::Item::List &items, const QString &username);
};

class TagChangeHelper : public QObject
{
    Q_OBJECT
public:
    explicit TagChangeHelper(KolabRelationResourceTask *parent = nullptr);

    void start(const Akonadi::Tag &tag, const KMime::Message::Ptr &message, KIMAP::Session *session);

Q_SIGNALS:
    void applyCollectionChanges(const Akonadi::Collection &collection);
    void cancelTask(const QString &errorText);
    void changeCommitted();

private:
    KolabRelationResourceTask *const mTask;

private:
    void recordNewUid(qint64 newUid, Akonadi::Tag tag);

private Q_SLOTS:
    void onReplaceDone(KJob *job);
    void onModifyDone(KJob *job);
};

#endif // TAGCHANGEHELPER_H
