/*
    SPDX-FileCopyrightText: 2009 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DELETEDITEMSATTRIBUTE_H
#define DELETEDITEMSATTRIBUTE_H

#include <attribute.h>
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

    ~DeletedItemsAttribute();

    void addDeletedItemOffset(quint64);

    DeletedItemsAttribute *clone() const override;

    QSet<quint64> deletedItemOffsets() const;
    KMBox::MBoxEntry::List deletedItemEntries() const;

    void deserialize(const QByteArray &data) override;

    /**
     * Returns the number of offsets stored in this attribute.
     */
    int offsetCount() const;

    QByteArray serialized() const override;

    QByteArray type() const override;

    bool operator ==(const DeletedItemsAttribute &other) const;
private:
    QSet<quint64> mDeletedItemOffsets;
};

#endif
