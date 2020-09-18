/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef COLLECTIONCACHEATTRIBUTE_H
#define COLLECTIONCACHEATTRIBUTE_H

#include "etebaseadapter.h"
#include <AkonadiCore/Attribute>

class CollectionCacheAttribute : public Akonadi::Attribute
{
public:
    explicit CollectionCacheAttribute(QByteArray collectionCache = nullptr);
    void setCollectionCache(const QByteArray &collectionCache);
    QByteArray collectionCache() const;
    QByteArray type() const override;
    Attribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QByteArray mCollectionCache;
};

#endif
