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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "commonconversion.h"
#include "pimkolab_debug.h"
#include "timezoneconverter.h"
#include "utils/porting.h"

#include <iostream>
#include <KTimeZone>
#include <QUrl>
#include <QTimeZone>

namespace Kolab {
namespace Conversion {
QTimeZone getTimeZone(const std::string &timezone)
{
    //Convert non-olson timezones if necessary
    const QString normalizedTz = TimezoneConverter::normalizeTimezone(QString::fromStdString(timezone));
    if (!QTimeZone::isTimeZoneIdAvailable(normalizedTz.toLatin1())) {
        return QTimeZone::systemTimeZone();
    }
    //FIXME convert this to a proper KTimeZone
    return QTimeZone(normalizedTz.toLatin1());
}

QTimeZone getTimeSpec(bool isUtc, const std::string &timezone)
{
    if (isUtc) { //UTC
        return QTimeZone::utc();
    }
    if (timezone.empty()) { //Floating
        return QTimeZone::systemTimeZone();
    }

    //Convert non-olson timezones if necessary
    QTimeZone tz = getTimeZone(timezone);
    if (!tz.isValid()) {
        return QTimeZone::systemTimeZone();
    }
    return tz;
}

QDateTime toDate(const Kolab::cDateTime &dt)
{
    QDateTime date;
    if (!dt.isValid()) {
        return QDateTime();
    }
    if (dt.isDateOnly()) {
        date.setDate(QDate(dt.year(), dt.month(), dt.day()));
        date.setTime({});
        date.setTimeSpec(Qt::LocalTime);
    } else {
        date.setDate(QDate(dt.year(), dt.month(), dt.day()));
        date.setTime(QTime(dt.hour(), dt.minute(), dt.second()));
        if (dt.isUTC()) {
            date.setTimeSpec(Qt::UTC);
        } else {
            date.setTimeZone(getTimeZone(dt.timezone()));
        }
    }
    Q_ASSERT(date.isValid());
    return date;
}

cDateTime fromDate(const KDateTime &dt)
{
    if (!dt.isValid()) {
        return cDateTime();
    }
    cDateTime date;
    if (dt.isDateOnly()) { //Date only
        const QDate &d = dt.date();
        date.setDate(d.year(), d.month(), d.day());
    } else {
        const QDate &d = dt.date();
        date.setDate(d.year(), d.month(), d.day());
        const QTime &t = dt.time();
        date.setTime(t.hour(), t.minute(), t.second());
        if (dt.timeType() == KDateTime::UTC) { //UTC
            date.setUTC(true);
        } else if (dt.timeType() == KDateTime::OffsetFromUTC) {
            const KDateTime utcDate = dt.toUtc();
            const QDate &d = utcDate.date();
            date.setDate(d.year(), d.month(), d.day());
            const QTime &t = utcDate.time();
            date.setTime(t.hour(), t.minute(), t.second());
            date.setUTC(true);
        } else if (dt.timeType() == KDateTime::TimeZone) { //Timezone
            //TODO handle local timezone?
            //Convert non-olson timezones if necessary
            const QString timezone = TimezoneConverter::normalizeTimezone(dt.timeZone().name());
            if (!timezone.isEmpty()) {
                date.setTimezone(toStdString(timezone));
            } else {
                qCWarning(PIMKOLAB_LOG) << "invalid timezone: " << dt.timeZone().name() << ", assuming floating time";
                return date;
            }
        } else if (dt.timeType() != KDateTime::ClockTime) {
            qCCritical(PIMKOLAB_LOG) << "invalid timespec, assuming floating time. Type: " << dt.timeType() << "dt: " << dt.toString();
            return date;
        }
    }
    Q_ASSERT(date.isValid());
    return date;
}

cDateTime fromDate(const QDateTime &dt, bool isAllDay)
{
    if (!dt.isValid()) {
        return cDateTime();
    }
    cDateTime date;
    if (isAllDay) {
        const QDate &d = dt.date();
        date.setDate(d.year(), d.month(), d.day());
    } else {
        const QDate &d = dt.date();
        date.setDate(d.year(), d.month(), d.day());
        const QTime &t = dt.time();
        date.setTime(t.hour(), t.minute(), t.second());
        if (dt.timeSpec() == Qt::UTC) {
            date.setUTC(true);
        } else if (dt.timeSpec() == Qt::OffsetFromUTC) {
            const QDateTime utcDate = dt.toUTC();
            const QDate &d = utcDate.date();
            date.setDate(d.year(), d.month(), d.day());
            const QTime &t = utcDate.time();
            date.setTime(t.hour(), t.minute(), t.second());
            date.setUTC(true);
        } else if (dt.timeSpec() == Qt::TimeZone) {
            const QString timezone = TimezoneConverter::normalizeTimezone(QString::fromUtf8(dt.timeZone().id()));
            if (!timezone.isEmpty()) {
                date.setTimezone(toStdString(timezone));
            } else {
                qCWarning(PIMKOLAB_LOG) << "invalid timezone: " << dt.timeZone().id() << ", assuming floating time";
                return date;
            }
        }
    }
    Q_ASSERT(date.isValid());
    return date;
}

QStringList toStringList(const std::vector<std::string> &l)
{
    QStringList list;
    list.reserve(l.size());
    for (const std::string &s : l) {
        list.append(Conversion::fromStdString(s));
    }
    return list;
}

std::vector<std::string> fromStringList(const QStringList &l)
{
    std::vector<std::string> list;
    list.reserve(l.size());
    for (const QString &s : l) {
        list.push_back(toStdString(s));
    }
    return list;
}

QUrl toMailto(const std::string &email, const std::string &name)
{
    std::string mailto;
    if (!name.empty()) {
        mailto.append(name);
    }
    mailto.append("<");
    mailto.append(email);
    mailto.append(">");
    return QUrl(QString::fromStdString(std::string("mailto:")+mailto));
}

std::string fromMailto(const QUrl &mailtoUri, std::string &name)
{
    const QPair<std::string, std::string> pair = fromMailto(toStdString(mailtoUri.toString()));
    name = pair.second;
    return pair.first;
}

QPair<std::string, std::string> fromMailto(const std::string &mailto)
{
    const std::string &decoded = toStdString(QUrl::fromPercentEncoding(QByteArray(mailto.c_str())));
    if (decoded.substr(0, 7).compare("mailto:")) {
        // WARNING("no mailto address");
        // std::cout << decoded << std::endl;
        return qMakePair(decoded, std::string());
    }
    std::size_t begin = decoded.find('<', 7);
    if (begin == std::string::npos) {
        qCDebug(PIMKOLAB_LOG) << "no mailto address";
        std::cout << decoded << std::endl;
        return qMakePair(decoded, std::string());
    }
    std::size_t end = decoded.find('>', begin);
    if (end == std::string::npos) {
        qCWarning(PIMKOLAB_LOG) << "no mailto address";
        std::cout << decoded << std::endl;
        return qMakePair(decoded, std::string());
    }
    const std::string name = decoded.substr(7, begin-7);
    const std::string email = decoded.substr(begin+1, end-begin-1);
    return qMakePair(email, name);
}
}
}
