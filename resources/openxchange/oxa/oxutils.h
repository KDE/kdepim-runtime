/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QString>

namespace OXA
{
namespace OXUtils
{
[[nodiscard]] QString writeBoolean(bool value);
[[nodiscard]] QString writeNumber(qlonglong value);
[[nodiscard]] QString writeString(const QString &value);
[[nodiscard]] QString writeName(const QString &value);
[[nodiscard]] QString writeDateTime(const QDateTime &value);
[[nodiscard]] QString writeDate(QDate value);

[[nodiscard]] bool readBoolean(const QString &text);
[[nodiscard]] qlonglong readNumber(const QString &text);
[[nodiscard]] QString readString(const QString &text);
[[nodiscard]] QString readName(const QString &text);
[[nodiscard]] QDateTime readDateTime(const QString &text);
[[nodiscard]] QDate readDate(const QString &text);
}
}
