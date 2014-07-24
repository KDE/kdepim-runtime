/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef GMAILLABELATTRIBUTE_H
#define GMAILLABELATTRIBUTE_H

#include <Akonadi/Attribute>
#include <QByteArray>

class GmailLabelAttribute : public Akonadi::Attribute
{
public:
    explicit GmailLabelAttribute();
    explicit GmailLabelAttribute(const QByteArray &label);
    virtual ~GmailLabelAttribute();

    QByteArray label() const;
    void setLabel(const QByteArray &label);

    bool isDrafts() const;
    bool isTrash() const;
    bool isImportant() const;
    bool isSent() const;
    bool isJunk() const;
    bool isFlagged() const;
    bool isInbox() const;
    bool isAllMail() const;

    void deserialize(const QByteArray &data);
    QByteArray serialized() const;
    Akonadi::Attribute* clone() const;
    QByteArray type() const;

private:
    QByteArray mLabel;
};

#endif // GMAILLABELATTRIBUTE_H
