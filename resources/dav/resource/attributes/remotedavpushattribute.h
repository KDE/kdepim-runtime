/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Attribute>

#include <QString>

class RemoteDavPushAttribute : public Akonadi::Attribute
{
public:
    explicit RemoteDavPushAttribute();

    [[nodiscard]] QString topic() const;
    void setTopic(const QString &topic);

    [[nodiscard]] Akonadi::Attribute *clone() const override;
    [[nodiscard]] QByteArray type() const override;
    [[nodiscard]] QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QString mTopic;
};
