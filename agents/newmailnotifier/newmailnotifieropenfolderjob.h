/*
   SPDX-FileCopyrightText: 2023-2024 Laurent Montel <montel.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KJob>
#include <QObject>

class NewMailNotifierOpenFolderJob : public KJob
{
    Q_OBJECT
public:
    explicit NewMailNotifierOpenFolderJob(const QString &identifier, QObject *parent = nullptr);
    ~NewMailNotifierOpenFolderJob() override;

    void start() override;

private:
    const QString mIdentifer;
};
