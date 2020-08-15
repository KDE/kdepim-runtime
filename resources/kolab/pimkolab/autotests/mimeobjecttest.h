/*
 * SPDX-FileCopyrightText: 2012 Sofia Balicka <balicka@kolabsys.com>
 * SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
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

private Q_SLOTS:
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
