/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QObject>

class NewMailHistoryNotificationManager : public QObject
{
    Q_OBJECT
public:
    explicit NewMailHistoryNotificationManager(QObject *parent = nullptr);
    ~NewMailHistoryNotificationManager() override;

    static NewMailHistoryNotificationManager *self();

    Q_REQUIRED_RESULT QStringList history() const;
    void setHistory(const QStringList &newHistory);

    void addHistory(QString str);

    void clear();

Q_SIGNALS:
    void historyAdded(const QString &str);

private:
    QStringList mHistory;
};
