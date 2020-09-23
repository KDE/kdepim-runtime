/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ETEBASECACHEATTRIBUTE_H
#define ETEBASECACHEATTRIBUTE_H

#include "etebaseadapter.h"
#include <AkonadiCore/Attribute>

class EtebaseCacheAttribute : public Akonadi::Attribute
{
public:
    explicit EtebaseCacheAttribute(QByteArray etebaseCache = QByteArray());

    void setEtebaseCache(const QByteArray etebaseCache);
    QByteArray etebaseCache() const;

    QByteArray type() const override;
    Attribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QByteArray mEtebaseCache;
};

#endif
