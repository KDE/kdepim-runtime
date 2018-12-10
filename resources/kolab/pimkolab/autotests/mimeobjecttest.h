/*
 * Copyright (C) 2012  Sofia Balicka <balicka@kolabsys.com>
 * Copyright (C) 2014  Christian Mollekopf <mollekopf@kolabsys.com>
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

#ifndef MIMEOBJECTTEST_H
#define MIMEOBJECTTEST_H
#include <QObject>

/**
 * Read-Write roundtrip test.
 *
 * Ensures we can read a mime message, serialize it again, and contents are still the same.
 */
class MIMEObjectTest : public QObject
{
    Q_OBJECT

private slots:
    void testEvent_data();
    void testEvent();
    void testTodo_data();
    void testTodo();
    void testJournal_data();
    void testJournal();
    void testNote_data();
    void testNote();
    void testContact_data();
    void testContact();
    void testDistlist_data();
    void testDistlist();
};
#endif // MIMEOBJECTTEST_H
