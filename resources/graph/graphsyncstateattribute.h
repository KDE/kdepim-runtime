/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Custom attribute to keep the Graph delta sync state with the collection.
    Stores the @odata.deltaLink returned by the last messages/delta round, so the
    next sync only retrieves changes. Mirrors EwsSyncStateAttribute.
*/

#pragma once

#include <Akonadi/Attribute>

class GraphSyncStateAttribute : public Akonadi::Attribute
{
public:
    GraphSyncStateAttribute() = default;
    explicit GraphSyncStateAttribute(const QString &deltaLink);

    void setDeltaLink(const QString &deltaLink);
    [[nodiscard]] const QString &deltaLink() const;

    QByteArray type() const override;
    Attribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QString mDeltaLink;
};
