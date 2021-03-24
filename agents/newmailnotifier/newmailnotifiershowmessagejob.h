/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <AkonadiCore/Item>
#include <KJob>

class NewMailNotifierShowMessageJob : public KJob
{
    Q_OBJECT
public:
    explicit NewMailNotifierShowMessageJob(Akonadi::Item::Id id, QObject *parent = nullptr);
    ~NewMailNotifierShowMessageJob() override;

    void start() override;

private:
    const Akonadi::Item::Id mId;
};

