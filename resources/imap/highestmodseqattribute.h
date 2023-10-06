/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#pragma once

#include <Akonadi/Attribute>

class HighestModSeqAttribute : public Akonadi::Attribute
{
public:
    explicit HighestModSeqAttribute(qint64 highestModSequence = -1);
    void setHighestModSeq(qint64 highestModSequence);
    [[nodiscard]] qint64 highestModSequence() const;

    void deserialize(const QByteArray &data) override;
    [[nodiscard]] QByteArray serialized() const override;
    Akonadi::Attribute *clone() const override;
    [[nodiscard]] QByteArray type() const override;

private:
    qint64 m_highestModSeq;
};
