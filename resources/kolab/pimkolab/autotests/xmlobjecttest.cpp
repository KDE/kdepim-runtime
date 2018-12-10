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

#include "xmlobjecttest.h"

#include <QTest>
#include <iostream>

#include "kolabformat/xmlobject.h"

void XMLObjectTest::testEvent()
{
    Kolab::Event event;
    event.setStart(Kolab::cDateTime(2012, 01, 01));

    Kolab::XMLObject xmlobject;
    const std::string output = xmlobject.writeEvent(event, Kolab::KolabV2, "productid");
    QVERIFY(!output.empty());
    std::cout << output;

    const Kolab::Event resultEvent = xmlobject.readEvent(output, Kolab::KolabV2);
    QVERIFY(resultEvent.isValid());
}

void XMLObjectTest::testDontCrash()
{
    Kolab::XMLObject ob;
    ob.writeEvent(Kolab::Event(), Kolab::KolabV2, "");
    ob.writeTodo(Kolab::Todo(), Kolab::KolabV2, "");
    ob.writeJournal(Kolab::Journal(), Kolab::KolabV2, "");
    ob.writeFreebusy(Kolab::Freebusy(), Kolab::KolabV2, "");
    ob.writeContact(Kolab::Contact(), Kolab::KolabV2, "");
    ob.writeDistlist(Kolab::DistList(), Kolab::KolabV2, "");
    ob.writeNote(Kolab::Note(), Kolab::KolabV2, "");
    ob.writeConfiguration(Kolab::Configuration(), Kolab::KolabV2, "");

    ob.readEvent("", Kolab::KolabV2);
    ob.readTodo("", Kolab::KolabV2);
    ob.readJournal("", Kolab::KolabV2);
    ob.readFreebusy("", Kolab::KolabV2);
    ob.readContact("", Kolab::KolabV2);
    ob.readDistlist("", Kolab::KolabV2);
    ob.readNote("", Kolab::KolabV2);
    ob.readConfiguration("", Kolab::KolabV2);
}

QTEST_MAIN(XMLObjectTest)
