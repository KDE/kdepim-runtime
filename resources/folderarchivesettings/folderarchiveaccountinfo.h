/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FOLDERARCHIVEACCOUNTINFO_H
#define FOLDERARCHIVEACCOUNTINFO_H

#include <KConfigGroup>
#include <AkonadiCore/Collection>

class FolderArchiveAccountInfo
{
public:
    FolderArchiveAccountInfo();
    explicit FolderArchiveAccountInfo(const KConfigGroup &config);
    ~FolderArchiveAccountInfo();

    enum FolderArchiveType {
        UniqueFolder,
        FolderByMonths,
        FolderByYears
    };

    bool isValid() const;

    QString instanceName() const;
    void setInstanceName(const QString &instance);

    void setArchiveTopLevel(Akonadi::Collection::Id id);
    Akonadi::Collection::Id archiveTopLevel() const;

    void setFolderArchiveType(FolderArchiveType type);
    FolderArchiveType folderArchiveType() const;

    void setEnabled(bool enabled);
    bool enabled() const;

    void setKeepExistingStructure(bool b);
    bool keepExistingStructure() const;

    void writeConfig(KConfigGroup &config);
    void readConfig(const KConfigGroup &config);

    bool operator==(const FolderArchiveAccountInfo &other) const;

private:
    FolderArchiveAccountInfo::FolderArchiveType mArchiveType = UniqueFolder;
    Akonadi::Collection::Id mArchiveTopLevelCollectionId = -1;
    QString mInstanceName;
    bool mEnabled = false;
    bool mKeepExistingStructure = false;
};

#endif // FOLDERARCHIVEACCOUNTINFO_H
