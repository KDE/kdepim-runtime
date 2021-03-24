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
QString writeBoolean(bool value);
QString writeNumber(qlonglong value);
QString writeString(const QString &value);
QString writeName(const QString &value);
QString writeDateTime(const QDateTime &value);
QString writeDate(const QDate &value);

bool readBoolean(const QString &text);
qlonglong readNumber(const QString &text);
QString readString(const QString &text);
QString readName(const QString &text);
QDateTime readDateTime(const QString &text);
QDate readDate(const QString &text);
}
}

