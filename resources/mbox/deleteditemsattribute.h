/*
    SPDX-FileCopyrightText: 2009 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Attribute>
#include <KMbox/MBoxEntry>

#include <QSet>

/**
 * This attribute stores a list of offsets in the mbox file of mails which are
 * deleted but not yet actually removed from the file yet.
 */
class DeletedItemsAttribute : public Akonadi::Attribute
{
public:
    DeletedItemsAttribute();

    DeletedItemsAttribute(const DeletedItemsAttribute &other);

    ~DeletedItemsAttribute() override;

    void addDeletedItemOffset(quint64);

    DeletedItemsAttribute *clone() const override;

    [[nodiscard]] QSet<quint64> deletedItemOffsets() const;
    [[nodiscard]] KMBox::MBoxEntry::List deletedItemEntries() const;

    void deserialize(const QByteArray &data) override;

    /**
     * Returns the number of offsets stored in this attribute.
     */
    [[nodiscard]] int offsetCount() const;

    [[nodiscard]] QByteArray serialized() const override;

    [[nodiscard]] QByteArray type() const override;

    [[nodiscard]] bool operator==(const DeletedItemsAttribute &other) const;

private:
    QSet<quint64> mDeletedItemOffsets;
};
