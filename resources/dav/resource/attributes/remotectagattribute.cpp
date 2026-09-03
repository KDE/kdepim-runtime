/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "remotectagattribute.h"

RemoteCTagAttribute::RemoteCTagAttribute(const QString &ctag)
    : mCTag(ctag)
{
}

void RemoteCTagAttribute::setCTag(const QString &ctag)
{
    mCTag = ctag;
}

QString RemoteCTagAttribute::CTag() const
{
    return mCTag;
}

Akonadi::Attribute *RemoteCTagAttribute::clone() const
{
    return new RemoteCTagAttribute(mCTag);
}

QByteArray RemoteCTagAttribute::type() const
{
    static const QByteArray sType("remote-ctag");
    return sType;
}

QByteArray RemoteCTagAttribute::serialized() const
{
    return mCTag.toUtf8();
}

void RemoteCTagAttribute::deserialize(const QByteArray &data)
{
    mCTag = QString::fromUtf8(data);
}
