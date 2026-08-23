/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphsyncstateattribute.h"

GraphSyncStateAttribute::GraphSyncStateAttribute(const QString &deltaLink)
    : mDeltaLink(deltaLink)
{
}

void GraphSyncStateAttribute::setDeltaLink(const QString &deltaLink)
{
    mDeltaLink = deltaLink;
}

const QString &GraphSyncStateAttribute::deltaLink() const
{
    return mDeltaLink;
}

QByteArray GraphSyncStateAttribute::type() const
{
    return QByteArrayLiteral("graphsyncstate");
}

Akonadi::Attribute *GraphSyncStateAttribute::clone() const
{
    return new GraphSyncStateAttribute(mDeltaLink);
}

QByteArray GraphSyncStateAttribute::serialized() const
{
    return mDeltaLink.toUtf8();
}

void GraphSyncStateAttribute::deserialize(const QByteArray &data)
{
    mDeltaLink = QString::fromUtf8(data);
}
