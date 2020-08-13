/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef DEBUGSTREAMTEST_H
#define DEBUGSTREAMTEST_H

#include <QObject>

class DebugStreamTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDebugstream();
    void testDebugNotLogged();
    void testHasError();
};

#endif // DEBUGSTREAMTEST_H
