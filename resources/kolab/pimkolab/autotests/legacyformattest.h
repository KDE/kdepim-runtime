/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef LEGACYFORMATTEST_H
#define LEGACYFORMATTEST_H

#include <QObject>

class V2Test : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testReadDistlistUID();
    void testWriteDistlistUID();
};

#endif // LEGACYFORMATTEST_H
