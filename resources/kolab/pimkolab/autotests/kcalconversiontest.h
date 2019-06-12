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

#ifndef KCALCONVERSIONTEST_H
#define KCALCONVERSIONTEST_H

#include <QObject>
#include <QtTest>

class KCalConversionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();

    void testDate_data();
    void testDate();

    void testDuration_data();
    void testDuration();

    void testConversion_data();
    void testConversion();

    void testTodoConversion_data();
    void testTodoConversion();

    void testJournalConversion_data();
    void testJournalConversion();

    void testContactConversion_data();
    void testContactConversion();

    void testDateTZ_data();
    void testDateTZ();
};

#endif
