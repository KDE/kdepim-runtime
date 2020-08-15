/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOLABMESSAGEHELPER_H
#define KOLABMESSAGEHELPER_H

#include <messagehelper.h>
#include <AkonadiCore/Collection>

class KolabMessageHelper : public MessageHelper
{
public:
    explicit KolabMessageHelper(const Akonadi::Collection &collection);
    ~KolabMessageHelper() override;
    Akonadi::Item createItemFromMessage(const KMime::Message::Ptr &message, const qint64 uid, const qint64 size, const QMap<QByteArray, QVariant> &attrs, const QList<QByteArray> &flags, const KIMAP::FetchJob::FetchScope &scope, bool &ok) const override;

private:
    Akonadi::Collection mCollection;
};

#endif
