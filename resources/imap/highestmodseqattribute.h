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

#ifndef HIGHESTMODSEQATTRIBUTE_H
#define HIGHESTMODSEQATTRIBUTE_H

#include <Akonadi/Attribute>

class HighestModSeqAttribute : public Akonadi::Attribute
{
  public:
    explicit HighestModSeqAttribute( qint64 highestModSequence = -1 );
    void setHighestModSeq( qint64 highestModSequence );
    qint64 highestModSequence() const;

    virtual void deserialize(const QByteArray& data);
    virtual QByteArray serialized() const;
    virtual Akonadi::Attribute* clone() const;
    virtual QByteArray type() const;

  private:
    qint64 m_highestModSeq;
};

#endif // HIGHESTMODSEQATTRIBUTE_H
