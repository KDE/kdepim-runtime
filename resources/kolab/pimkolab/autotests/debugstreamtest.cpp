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

#include "debugstreamtest.h"

#include "kolabformat/errorhandler.h"

#include <QTest>

void DebugStreamTest::testDebugstream()
{
    Error() << "test1";
    Error() << "test2" << "bla" << 3 << QMap<QString, int>();
    QCOMPARE(Kolab::ErrorHandler::instance().getErrors().size(), 2);
    QVERIFY(Kolab::ErrorHandler::instance().getErrors().first().message.contains("test1"));
    QCOMPARE(Kolab::ErrorHandler::instance().getErrors().first().severity, Kolab::ErrorHandler::Error);
    QVERIFY(Kolab::ErrorHandler::instance().getErrors().last().message.contains("bla"));
}

void DebugStreamTest::testDebugNotLogged()
{
    Kolab::ErrorHandler::instance().clear();
    Debug() << "test1";
    QCOMPARE(Kolab::ErrorHandler::instance().getErrors().size(), 0);
}

void DebugStreamTest::testHasError()
{
    Debug() << "test1";
    QCOMPARE(Kolab::ErrorHandler::errorOccured(), false);
    Warning() << "test1";
    QCOMPARE(Kolab::ErrorHandler::errorOccured(), false);
    Error() << "test1";
    QCOMPARE(Kolab::ErrorHandler::errorOccured(), true);
    Kolab::ErrorHandler::clearErrors();
    QCOMPARE(Kolab::ErrorHandler::errorOccured(), false);
}

QTEST_MAIN(DebugStreamTest)
