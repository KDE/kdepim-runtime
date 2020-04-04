/*
    Copyright (c) 2009 Bertjan Broeksema <broeksema@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
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
