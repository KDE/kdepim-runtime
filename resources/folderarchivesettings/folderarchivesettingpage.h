/*
   SPDX-FileCopyrightText: 2013-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "folderarchiveaccountinfo.h"
#include "folderarchivesettings_export.h"
#include <QWidget>

class QCheckBox;
namespace Akonadi
{
class CollectionRequester;
}

class FolderArchiveAccountInfo;
class FolderArchiveComboBox;
class FOLDERARCHIVESETTINGS_EXPORT FolderArchiveSettingPage : public QWidget
{
    Q_OBJECT
public:
    explicit FolderArchiveSettingPage(const QString &instanceName, QWidget *parent = nullptr);
    ~FolderArchiveSettingPage() override;

    void loadSettings();
    void writeSettings();

private:
    FOLDERARCHIVESETTINGS_NO_EXPORT void slotEnableChanged(bool enabled);
    const QString mInstanceName;
    QCheckBox *const mEnabled;
    FolderArchiveComboBox *mArchiveNamed = nullptr;
    Akonadi::CollectionRequester *const mArchiveFolder;
    FolderArchiveAccountInfo *mInfo = nullptr;
};
