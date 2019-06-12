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

#include "legacyformattest.h"
#include "kolabformat/xmlobject.h"
#include "kolabformat/errorhandler.h"
#include "testutils.h"
#include "pimkolab_debug.h"
#include <QTest>
#include <fstream>
#include <sstream>

void V2Test::testReadDistlistUID()
{
    std::ifstream t((TESTFILEDIR.toStdString()+"v2/contacts/distlistWithUID.xml").c_str());
    std::stringstream buffer;
    buffer << t.rdbuf();

    Kolab::XMLObject xo;
    const Kolab::DistList distlist = xo.readDistlist(buffer.str(), Kolab::KolabV2);
    foreach (const Kolab::ContactReference &contact, distlist.members()) {
        QVERIFY(!contact.uid().empty());
    }
    QVERIFY(!Kolab::ErrorHandler::errorOccured());
}

void V2Test::testWriteDistlistUID()
{
    Kolab::DistList distlist;
    distlist.setUid("uid");
    distlist.setName("name");
    std::vector<Kolab::ContactReference> members;
    members.push_back(Kolab::ContactReference(Kolab::ContactReference::UidReference, "memberuid", "membername"));
    distlist.setMembers(members);

    Kolab::XMLObject xo;
    const std::string xml = xo.writeDistlist(distlist, Kolab::KolabV2);
    QVERIFY(QString::fromStdString(xml).contains(QLatin1String("memberuid")));
    QVERIFY(!Kolab::ErrorHandler::errorOccured());
}

QTEST_MAIN(V2Test)
