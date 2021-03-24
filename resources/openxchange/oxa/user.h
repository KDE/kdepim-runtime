/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QString>
#include <QVector>

namespace OXA
{
class User
{
public:
    typedef QVector<User> List;

    User();

    bool isValid() const;

    void setUid(qlonglong uid);
    qlonglong uid() const;

    void setEmail(const QString &email);
    QString email() const;

    void setName(const QString &name);
    QString name() const;

private:
    qlonglong mUid = -1;
    QString mEmail;
    QString mName;
};
}

Q_DECLARE_TYPEINFO(OXA::User, Q_MOVABLE_TYPE);
