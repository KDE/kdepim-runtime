/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "synctokenattribute.h"

SyncTokenAttribute::SyncTokenAttribute(const QString &syncToken)
    : mSyncToken(syncToken)
{
}

void SyncTokenAttribute::setSyncToken(const QString &syncToken)
{
    mSyncToken = syncToken;
}

QString SyncTokenAttribute::syncToken() const
{
    return mSyncToken;
}

Akonadi::Attribute *SyncTokenAttribute::clone() const
{
    return new SyncTokenAttribute(syncToken());
}

QByteArray SyncTokenAttribute::type() const
{
    static const QByteArray sType("sync-token");
    return sType;
}

QByteArray SyncTokenAttribute::serialized() const
{
    return mSyncToken.toUtf8();
}

void SyncTokenAttribute::deserialize(const QByteArray &data)
{
    mSyncToken = QString::fromUtf8(data);
}
