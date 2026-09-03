/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Attribute>

#include <QString>

class RemoteCTagAttribute : public Akonadi::Attribute
{
public:
    explicit RemoteCTagAttribute(const QString &ctag = QString());

    void setCTag(const QString &ctag);
    QString CTag() const;

    Akonadi::Attribute *clone() const override;
    QByteArray type() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QString mCTag;
};
