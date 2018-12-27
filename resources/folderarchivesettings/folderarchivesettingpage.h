/*
   Copyright (C) 2013-2019 Laurent Montel <montel@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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
    QString mInstanceName;
    QCheckBox *mEnabled = nullptr;
    FolderArchiveComboBox *mArchiveNamed = nullptr;
    Akonadi::CollectionRequester *mArchiveFolder = nullptr;
    FolderArchiveAccountInfo *mInfo = nullptr;
};

#endif // FOLDERARCHIVESETTINGPAGE_H
