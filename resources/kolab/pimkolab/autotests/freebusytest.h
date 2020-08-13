/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef FREEBUSYTEST_H
#define FREEBUSYTEST_H
#include <QObject>

class FreebusyTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void testFB_data();
    void testFB();
};

#endif // FREEBUSYTEST_H
