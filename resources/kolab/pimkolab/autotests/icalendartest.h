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

#ifndef ICALENDARTEST_H
#define ICALENDARTEST_H
#include <QObject>

class ICalendarTest : public QObject
{
    Q_OBJECT
private slots:

//     void testEventConflict_data();
    void testToICal();
    void testFromICalEvent();

    void testToITip();
    void testToIMip();
};

#endif // ICALENDARTEST_H
