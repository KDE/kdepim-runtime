/*
  Copyright (c) 2015-2017 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "deleteitemsattributetest.h"
#include "../deleteditemsattribute.h"
#include <qtest.h>

DeleteItemsAttributeTest::DeleteItemsAttributeTest(QObject *parent)
    : QObject(parent)
{

}

DeleteItemsAttributeTest::~DeleteItemsAttributeTest()
{

}

void DeleteItemsAttributeTest::shouldHaveDefaultValue()
{
    DeletedItemsAttribute attr;
    QVERIFY(attr.deletedItemOffsets().isEmpty());
    QCOMPARE(attr.offsetCount(), 0);
}

void DeleteItemsAttributeTest::shouldAssignValue()
{
    DeletedItemsAttribute attr;
    QSet<quint64> lst;
    lst.insert(15);
    attr.addDeletedItemOffset(15);
    lst.insert(154);
    attr.addDeletedItemOffset(154);
    lst.insert(225);
    attr.addDeletedItemOffset(225);
    QVERIFY(!attr.deletedItemOffsets().isEmpty());
    QCOMPARE(attr.offsetCount(), 3);
    QCOMPARE(lst, attr.deletedItemOffsets());
}

void DeleteItemsAttributeTest::shouldDeserializeValue()
{
    DeletedItemsAttribute attr;
    attr.addDeletedItemOffset(15);
    attr.addDeletedItemOffset(154);
    attr.addDeletedItemOffset(225);
    const QByteArray ba = attr.serialized();
    DeletedItemsAttribute result;
    result.deserialize(ba);
    QVERIFY(result == attr);
}

void DeleteItemsAttributeTest::shouldCloneAttribute()
{
    DeletedItemsAttribute attr;
    attr.addDeletedItemOffset(15);
    attr.addDeletedItemOffset(154);
    attr.addDeletedItemOffset(225);
    DeletedItemsAttribute *result = attr.clone();
    QVERIFY(*result == attr);
    delete result;
}

void DeleteItemsAttributeTest::shouldHaveTypeName()
{
    DeletedItemsAttribute attr;
    QCOMPARE(attr.type(), QByteArray("DeletedMboxItems"));
}

QTEST_MAIN(DeleteItemsAttributeTest)
