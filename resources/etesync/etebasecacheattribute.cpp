/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "etebaseadapter.h"
#include "etebasecacheattribute.h"
#include <AkonadiCore/Attribute>

EtebaseCacheAttribute::EtebaseCacheAttribute(QByteArray etebaseCache)
    : mEtebaseCache(etebaseCache)
{
}

void EtebaseCacheAttribute::setEtebaseCache(const QByteArray etebaseCache)
{
    mEtebaseCache = etebaseCache;
}

QByteArray EtebaseCacheAttribute::etebaseCache() const
{
    return mEtebaseCache;
}

QByteArray EtebaseCacheAttribute::type() const
{
    static const QByteArray sType("etebasecache");
    return sType;
}

Akonadi::Attribute *EtebaseCacheAttribute::clone() const
{
    return new EtebaseCacheAttribute(mEtebaseCache);
}

QByteArray EtebaseCacheAttribute::serialized() const
{
    return mEtebaseCache;
}

void EtebaseCacheAttribute::deserialize(const QByteArray &data)
{
    mEtebaseCache = data;
}
