/*
   SPDX-FileCopyrightText: 2021-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <KJob>

class NewMailNotifierReplyMessageJob : public KJob
{
    Q_OBJECT
public:
    explicit NewMailNotifierReplyMessageJob(Akonadi::Item::Id id, QObject *parent = nullptr);
    ~NewMailNotifierReplyMessageJob() override;

    void start() override;

    [[nodiscard]] bool replyToAll() const;
    void setReplyToAll(bool newReplyToAll);

private:
    const Akonadi::Item::Id mId;
    bool mReplyToAll = false;
};
