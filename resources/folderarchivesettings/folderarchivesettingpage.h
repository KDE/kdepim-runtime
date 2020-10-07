/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FOLDERARCHIVESETTINGPAGE_H
#define FOLDERARCHIVESETTINGPAGE_H

#include "folderarchiveaccountinfo.h"
#include "folderarchivesettings_export.h"
#include <QWidget>
#include <QComboBox>

class QCheckBox;
namespace Akonadi {
class CollectionRequester;
}

class FolderArchiveComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit FolderArchiveComboBox(QWidget *parent = nullptr);
    ~FolderArchiveComboBox();

    void setType(FolderArchiveAccountInfo::FolderArchiveType type);
    FolderArchiveAccountInfo::FolderArchiveType type() const;

private:
    void initialize();
};

class FolderArchiveAccountInfo;
class FOLDERARCHIVESETTINGS_EXPORT FolderArchiveSettingPage : public QWidget
{
    Q_OBJECT
public:
    explicit FolderArchiveSettingPage(const QString &instanceName, QWidget *parent = nullptr);
    ~FolderArchiveSettingPage();

    void loadSettings();
    void writeSettings();

private:
    void slotEnableChanged(bool enabled);
    const QString mInstanceName;
    QCheckBox *mEnabled = nullptr;
    FolderArchiveComboBox *mArchiveNamed = nullptr;
    Akonadi::CollectionRequester *mArchiveFolder = nullptr;
    FolderArchiveAccountInfo *mInfo = nullptr;
};

#endif // FOLDERARCHIVESETTINGPAGE_H
