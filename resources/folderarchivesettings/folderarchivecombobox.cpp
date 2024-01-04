/*
   SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "folderarchivecombobox.h"
#include <KLocalizedString>

FolderArchiveComboBox::FolderArchiveComboBox(QWidget *parent)
    : QComboBox(parent)
{
    initialize();
}

FolderArchiveComboBox::~FolderArchiveComboBox() = default;

void FolderArchiveComboBox::initialize()
{
    addItem(i18nc("@item:inlistbox for option \"Archive folder name\"", "Unique"), FolderArchiveAccountInfo::UniqueFolder);
    addItem(i18nc("@item:inlistbox for option \"Archive folder name\"", "Month and year"), FolderArchiveAccountInfo::FolderByMonths);
    addItem(i18nc("@item:inlistbox for option \"Archive folder name\"", "Year"), FolderArchiveAccountInfo::FolderByYears);
}

void FolderArchiveComboBox::setType(FolderArchiveAccountInfo::FolderArchiveType type)
{
    const int index = findData(static_cast<int>(type));
    if (index != -1) {
        setCurrentIndex(index);
    } else {
        setCurrentIndex(0);
    }
}

FolderArchiveAccountInfo::FolderArchiveType FolderArchiveComboBox::type() const
{
    return static_cast<FolderArchiveAccountInfo::FolderArchiveType>(itemData(currentIndex()).toInt());
}
