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

#ifndef TIMEZONETEST_H
#define TIMEZONETEST_H
#include <QObject>

class TimezoneTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    /**
     * If this unittest fails, many others will follow.
     */
    void initTestCase();

    void testFromName();
    void testFromHardcodedList_data();
    void testFromHardcodedList();
    void testKolabObjectWriter();
    // void testKolabObjectReader();
    void testFindLegacyTimezone();
    void testIgnoreInvalidTimezone();
    void testUTCOffset();
    void localTimezone();
};

#endif // TIMEZONETEST_H
