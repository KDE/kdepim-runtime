/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "highestmodseqattribute.h"

#include <QtCore/QByteArray>

HighestModSeqAttribute::HighestModSeqAttribute( qint64 highestModSequence ):
    Akonadi::Attribute(),
    m_highestModSeq( highestModSequence )
{
}

void HighestModSeqAttribute::setHighestModSeq( qint64 highestModSequence )
{
    m_highestModSeq = highestModSequence;
}

qint64 HighestModSeqAttribute::highestModSequence() const
{
    return m_highestModSeq;
}

Akonadi::Attribute* HighestModSeqAttribute::clone() const
{
    return new HighestModSeqAttribute( m_highestModSeq );
}

QByteArray HighestModSeqAttribute::type() const
{
    return "highestmodseq";
}

void HighestModSeqAttribute::deserialize( const QByteArray &data )
{
    m_highestModSeq = data.toLongLong();
}

QByteArray HighestModSeqAttribute::serialized() const
{
    return QByteArray::number( m_highestModSeq );
}
