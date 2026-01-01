/*
   SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "folderarchivecomboboxtest.h"
#include "folderarchivecombobox.h"
#include <QTest>

QTEST_MAIN(FolderArchiveComboBoxTest)

FolderArchiveComboBoxTest::FolderArchiveComboBoxTest(QObject *parent)
    : QObject{parent}
{
}

void FolderArchiveComboBoxTest::shouldHaveDefaultValues()
{
    FolderArchiveComboBox w;
    QCOMPARE(w.count(), 3);
    QCOMPARE(w.type(), FolderArchiveAccountInfo::UniqueFolder);
}

#include "moc_folderarchivecomboboxtest.cpp"
