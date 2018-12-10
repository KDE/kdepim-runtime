/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KOLABCOMMONCONVERSION_H
#define KOLABCOMMONCONVERSION_H

#include "kolab_export.h"

#include <QDateTime>
#include <QStringList>
#include <QUrl>
#include <kolabcontainers.h>

class QTimeZone;

namespace Kolab {
namespace Conversion {
KOLAB_EXPORT QDateTime toDate(const Kolab::cDateTime &dt);
KOLAB_EXPORT cDateTime fromDate(const QDateTime &dt, bool isAllDay);
QStringList toStringList(const std::vector<std::string> &l);
std::vector<std::string> fromStringList(const QStringList &l);
/**
 * Returns a UTC, Floating Time or Timezone
 */
QTimeZone getTimeZone(const std::string &timezone);
QTimeZone getTimeSpec(bool isUtc, const std::string &timezone);

QUrl toMailto(const std::string &email, const std::string &name = std::string());
std::string fromMailto(const QUrl &mailtoUri, std::string &name);
QPair<std::string, std::string> fromMailto(const std::string &mailto);

inline std::string toStdString(const QString &s)
{
    return std::string(s.toUtf8().constData());
}

inline QString fromStdString(const std::string &s)
{
    return QString::fromUtf8(s.c_str());
}
}
}

#endif
