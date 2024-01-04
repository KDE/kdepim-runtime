/*
   SPDX-FileCopyrightText: 2013-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "folderarchivesettings_private_export.h"
#include <Akonadi/Collection>
#include <KConfigGroup>

class FOLDERARCHIVESETTINGS_TESTS_EXPORT FolderArchiveAccountInfo
{
public:
    FolderArchiveAccountInfo();
    explicit FolderArchiveAccountInfo(const KConfigGroup &config);
    ~FolderArchiveAccountInfo();

    enum FolderArchiveType {
        UniqueFolder,
        FolderByMonths,
        FolderByYears,
    };

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] QString instanceName() const;
    void setInstanceName(const QString &instance);

    void setArchiveTopLevel(Akonadi::Collection::Id id);
    [[nodiscard]] Akonadi::Collection::Id archiveTopLevel() const;

    void setFolderArchiveType(FolderArchiveType type);
    [[nodiscard]] FolderArchiveType folderArchiveType() const;

    void setEnabled(bool enabled);
    [[nodiscard]] bool enabled() const;

    void setKeepExistingStructure(bool b);
    [[nodiscard]] bool keepExistingStructure() const;

    void writeConfig(KConfigGroup &config);
    void readConfig(const KConfigGroup &config);

    [[nodiscard]] bool operator==(const FolderArchiveAccountInfo &other) const;

    [[nodiscard]] bool useDateFromMessage() const;
    void setUseDateFromMessage(bool newUseDateFromMessage);

private:
    FolderArchiveAccountInfo::FolderArchiveType mArchiveType = UniqueFolder;
    Akonadi::Collection::Id mArchiveTopLevelCollectionId = -1;
    QString mInstanceName;
    bool mUseDateFromMessage = false;
    bool mEnabled = false;
    bool mKeepExistingStructure = false;
};
