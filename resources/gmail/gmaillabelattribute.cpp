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

#include "gmaillabelattribute.h"

GmailLabelAttribute::GmailLabelAttribute()
    : Akonadi::Attribute()
{
}

GmailLabelAttribute::GmailLabelAttribute(const QByteArray &label)
    : Akonadi::Attribute()
    , mLabel(label)
{
}


GmailLabelAttribute::~GmailLabelAttribute()
{
}

QByteArray GmailLabelAttribute::label() const
{
    return mLabel;
}

void GmailLabelAttribute::setLabel(const QByteArray &label)
{
    mLabel = label;
}

void GmailLabelAttribute::deserialize(const QByteArray &data)
{
    mLabel = data;
}

QByteArray GmailLabelAttribute::serialized() const
{
    return mLabel;
}

Akonadi::Attribute *GmailLabelAttribute::clone() const
{
    return new GmailLabelAttribute(mLabel);
}

QByteArray GmailLabelAttribute::type() const
{
    return "GmailLabel";
}

bool GmailLabelAttribute::isAllMail() const
{
    return mLabel == "\\All";
}

bool GmailLabelAttribute::isDrafts() const
{
    return mLabel == "\\Draft";
}

bool GmailLabelAttribute::isFlagged() const
{
    return mLabel == "\\Flagged";
}

bool GmailLabelAttribute::isImportant() const
{
    return mLabel == "\\Important";
}

bool GmailLabelAttribute::isInbox() const
{
    return mLabel == "\\Inbox";
}

bool GmailLabelAttribute::isJunk() const
{
    return mLabel == "\\Junk";
}

bool GmailLabelAttribute::isSent() const
{
    return mLabel == "\\Sent";
}

bool GmailLabelAttribute::isTrash() const
{
    return mLabel == "\\Trash";
}
