/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef XMLOBJECTTEST_H
#define XMLOBJECTTEST_H

#include <QObject>

class XMLObjectTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testEvent();
    void testDontCrash();
};

#endif // XMLOBJECTTEST_H
