/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef HIGHESTMODSEQATTRIBUTE_H
#define HIGHESTMODSEQATTRIBUTE_H

#include <Attribute>

class HighestModSeqAttribute : public Akonadi::Attribute
{
public:
    explicit HighestModSeqAttribute(qint64 highestModSequence = -1);
    void setHighestModSeq(qint64 highestModSequence);
    qint64 highestModSequence() const;

    void deserialize(const QByteArray &data) override;
    QByteArray serialized() const override;
    Akonadi::Attribute *clone() const override;
    QByteArray type() const override;

private:
    qint64 m_highestModSeq;
};

#endif // HIGHESTMODSEQATTRIBUTE_H
