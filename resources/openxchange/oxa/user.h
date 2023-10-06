/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QList>
#include <QString>

namespace OXA
{
class User
{
public:
    using List = QList<User>;

    User();

    [[nodiscard]] bool isValid() const;

    void setUid(qlonglong uid);
    [[nodiscard]] qlonglong uid() const;

    void setEmail(const QString &email);
    [[nodiscard]] QString email() const;

    void setName(const QString &name);
    [[nodiscard]] QString name() const;

private:
    qlonglong mUid = -1;
    QString mEmail;
    QString mName;
};
}

Q_DECLARE_TYPEINFO(OXA::User, Q_RELOCATABLE_TYPE);
