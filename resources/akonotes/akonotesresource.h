/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "maildirresource.h"

class AkonotesResource : public MaildirResource
{
    Q_OBJECT
public:
    explicit AkonotesResource(const QString &id);
    ~AkonotesResource() override;

    [[nodiscard]] QString defaultResourceType() override;

protected:
    [[nodiscard]] QString itemMimeType() const override;
};
