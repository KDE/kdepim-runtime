/*
    SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <Akonadi/Attribute>

class NoInferiorsAttribute : public Akonadi::Attribute
{
public:
    explicit NoInferiorsAttribute(bool noInferiors = false);
    void setNoInferiors(bool noInferiors);
    bool noInferiors() const;
    QByteArray type() const override;
    Attribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    bool mNoInferiors;
};
