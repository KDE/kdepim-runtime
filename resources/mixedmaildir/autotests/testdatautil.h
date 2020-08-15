/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TESTDATAUTIL_H
#define TESTDATAUTIL_H

class QString;
class QStringList;

namespace TestDataUtil {
enum FolderType {
    InvalidFolder,
    MaildirFolder,
    MBoxFolder
};

FolderType folderType(const QString &testDataName);

QStringList testDataNames();

bool installFolder(const QString &testDataName, const QString &installPath, const QString &folderName);
}

#endif
