/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <Akonadi/Item>
#include <QObject>

class NewMailNotificationHistoryManager : public QObject
{
    Q_OBJECT
public:
    struct HistoryMailInfo {
        QString message;
        Akonadi::Item::Id identifier;
    };

    struct HistoryFolderInfo {
        QString message;
        Akonadi::Collection::Id identifier;
    };

    explicit NewMailNotificationHistoryManager(QObject *parent = nullptr);
    ~NewMailNotificationHistoryManager() override;

    static NewMailNotificationHistoryManager *self();

    Q_REQUIRED_RESULT QStringList history() const;
    void setHistory(const QStringList &newHistory);

    // FIXME remove it.
    void addHistory(QString str);

    void clear();

    void addEmailInfoNotificationHistory(const NewMailNotificationHistoryManager::HistoryMailInfo &info);
    void addFoldersInfoNotificationHistory(const QList<NewMailNotificationHistoryManager::HistoryFolderInfo> &infos);

Q_SIGNALS:
    void historyAdded(const QString &str);

private:
    Q_REQUIRED_RESULT static QString generateOpenMailStr(Akonadi::Item::Id id);
    Q_REQUIRED_RESULT static QString generateOpenFolderStr(Akonadi::Collection::Id id);
    void cleanupStr(QString &str);
    QStringList mHistory;
};
Q_DECLARE_TYPEINFO(NewMailNotificationHistoryManager::HistoryMailInfo, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(NewMailNotificationHistoryManager::HistoryFolderInfo, Q_RELOCATABLE_TYPE);
