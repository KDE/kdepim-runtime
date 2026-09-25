/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "remotedavpushattribute.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

namespace Akonadi
{
class Attribute;
}
RemoteDavPushAttribute::RemoteDavPushAttribute() = default;

QString RemoteDavPushAttribute::topic() const
{
    return mTopic;
}

void RemoteDavPushAttribute::setTopic(const QString &topic)
{
    mTopic = topic;
}

Akonadi::Attribute *RemoteDavPushAttribute::clone() const
{
    auto res = new RemoteDavPushAttribute();
    res->mTopic = this->mTopic;
    return res;
}

QByteArray RemoteDavPushAttribute::type() const
{
    static const QByteArray sType("remote-davpush");
    return sType;
}

QByteArray RemoteDavPushAttribute::serialized() const
{
    auto res = QByteArray();
    auto out = QDataStream(&res, QIODevice::WriteOnly);
    out << mTopic;
    return res;
}

void RemoteDavPushAttribute::deserialize(const QByteArray &data)
{
    auto in = QDataStream(data);
    in >> mTopic;
}
