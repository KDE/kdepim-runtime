/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "remotesynctokenattribute.h"

RemoteSyncTokenAttribute::RemoteSyncTokenAttribute(const QString &syncToken)
    : mSyncToken(syncToken)
{
}

void RemoteSyncTokenAttribute::setSyncToken(const QString &syncToken)
{
    mSyncToken = syncToken;
}

QString RemoteSyncTokenAttribute::syncToken() const
{
    return mSyncToken;
}

Akonadi::Attribute *RemoteSyncTokenAttribute::clone() const
{
    return new RemoteSyncTokenAttribute(syncToken());
}

QByteArray RemoteSyncTokenAttribute::type() const
{
    static const QByteArray sType("remote-sync-token");
    return sType;
}

QByteArray RemoteSyncTokenAttribute::serialized() const
{
    return mSyncToken.toUtf8();
}

void RemoteSyncTokenAttribute::deserialize(const QByteArray &data)
{
    mSyncToken = QString::fromUtf8(data);
}
