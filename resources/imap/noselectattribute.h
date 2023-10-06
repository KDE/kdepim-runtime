/*
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Attribute>

class NoSelectAttribute : public Akonadi::Attribute
{
public:
    explicit NoSelectAttribute(bool noSelect = false);
    void setNoSelect(bool noSelect);
    [[nodiscard]] bool noSelect() const;
    [[nodiscard]] QByteArray type() const override;
    Attribute *clone() const override;
    [[nodiscard]] QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    bool mNoSelect;
};
