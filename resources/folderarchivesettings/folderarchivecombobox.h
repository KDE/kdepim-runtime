/*
   SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "folderarchiveaccountinfo.h"
#include "folderarchivesettings_private_export.h"

#include <QComboBox>

class FOLDERARCHIVESETTINGS_TESTS_EXPORT FolderArchiveComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit FolderArchiveComboBox(QWidget *parent = nullptr);
    ~FolderArchiveComboBox() override;

    void setType(FolderArchiveAccountInfo::FolderArchiveType type);
    [[nodiscard]] FolderArchiveAccountInfo::FolderArchiveType type() const;

private:
    void initialize();
};
