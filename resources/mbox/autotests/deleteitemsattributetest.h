/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DELETEITEMSATTRIBUTETEST_H
#define DELETEITEMSATTRIBUTETEST_H

#include <QObject>

class DeleteItemsAttributeTest : public QObject
{
    Q_OBJECT
public:
    explicit DeleteItemsAttributeTest(QObject *parent = nullptr);
    ~DeleteItemsAttributeTest();

private Q_SLOTS:
    void shouldHaveDefaultValue();
    void shouldDeserializeValue();
    void shouldAssignValue();
    void shouldCloneAttribute();
    void shouldHaveTypeName();
};

#endif // DELETEITEMSATTRIBUTETEST_H
