/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "etebaseadapter.h"
#include "collectioncacheattribute.h"
#include <AkonadiCore/Attribute>

CollectionCacheAttribute::CollectionCacheAttribute(QByteArray collectionCache)
    : mCollectionCache(collectionCache)
{
}

void CollectionCacheAttribute::setCollectionCache(const QByteArray &collectionCache)
{
    mCollectionCache = collectionCache;
}

QByteArray CollectionCacheAttribute::collectionCache() const
{
    return mCollectionCache;
}

QByteArray CollectionCacheAttribute::type() const
{
    static const QByteArray sType("etebasecollectioncache");
    return sType;
}

Akonadi::Attribute *CollectionCacheAttribute::clone() const
{
    return new CollectionCacheAttribute(mCollectionCache);
}

QByteArray CollectionCacheAttribute::serialized() const
{
    return mCollectionCache;
}

void CollectionCacheAttribute::deserialize(const QByteArray &data)
{
    mCollectionCache = data;
}
