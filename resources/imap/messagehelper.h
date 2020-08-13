/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef MESSAGEHELPER_H
#define MESSAGEHELPER_H

#include <AkonadiCore/Item>
#include <KMime/Message>
#include <kimap/FetchJob>

class MessageHelper
{
public:
    typedef QSharedPointer<MessageHelper> Ptr;

    virtual ~MessageHelper();
    virtual Akonadi::Item createItemFromMessage(const KMime::Message::Ptr &message, const qint64 uid, const qint64 size, const QMap<QByteArray, QVariant> &attrs, const QList<QByteArray> &flags, const KIMAP::FetchJob::FetchScope &scope, bool &ok) const;
};

#endif
