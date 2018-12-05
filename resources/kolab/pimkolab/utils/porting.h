/*
  This file is part of the kcalcore library.

  Copyright (c) 2017  Daniel Vrátil <dvratil@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef PORTING_H
#define PORTING_H

#include <QDateTime>
#include <KDateTime>

namespace Porting {
// TODO: PORTING-helper only, remove once KDateTime is gone
/**
 * Helpers to retain backwards compatibility of binary serialization.
 */
void serializeQDateTimeAsKDateTime(QDataStream &out, const QDateTime &dt);
void deserializeKDateTimeAsQDateTime(QDataStream &in, QDateTime & dt);

/** Convert a QTimeZone to a KDateTime::Spec */
KDateTime::Spec zoneToSpec(const QTimeZone &zone);

/** Convert a QTimeZone to a KDateTime::Spec */
QTimeZone specToZone(const KDateTime::Spec &spec);

/** Convert KDateTime to QDateTime, correctly preserves timespec */
QDateTime k2q(const KDateTime &kdt);
KDateTime q2k(const QDateTime &qdt, bool isAllDay = false);
}

#endif // PORTING_H
