/*
    SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "newmailnotifier_export.h"
#include <Akonadi/Item>
#include <QDebug>
#include <QObject>

class NEWMAILNOTIFIER_EXPORT NewMailNotificationHistoryManager : public QObject
{
    Q_OBJECT
public:
    struct NEWMAILNOTIFIER_EXPORT HistoryMailInfo {
        QString message;
        Akonadi::Item::Id identifier;
    };

    struct NEWMAILNOTIFIER_EXPORT HistoryFolderInfo {
        QString message;
        Akonadi::Collection::Id identifier;
    };

    explicit NewMailNotificationHistoryManager(QObject *parent = nullptr);
    ~NewMailNotificationHistoryManager() override;

    static NewMailNotificationHistoryManager *self();

    [[nodiscard]] QStringList history() const;
    void setHistory(const QStringList &newHistory);

    void clear();

    void addEmailInfoNotificationHistory(const NewMailNotificationHistoryManager::HistoryMailInfo &info);
    void addFoldersInfoNotificationHistory(const QList<NewMailNotificationHistoryManager::HistoryFolderInfo> &infos);

    void setTestModeEnabled(bool test);
    [[nodiscard]] QString joinHistory() const;

Q_SIGNALS:
    void historyAdded(const QString &str);

private:
    [[nodiscard]] static NEWMAILNOTIFIER_NO_EXPORT QString generateOpenMailStr(Akonadi::Item::Id id);
    [[nodiscard]] static NEWMAILNOTIFIER_NO_EXPORT QString generateOpenFolderStr(Akonadi::Collection::Id id);
    NEWMAILNOTIFIER_NO_EXPORT void addHeader();
    QStringList mHistory;
    // Only for autotest
    bool mTestEnabled = false;
};
Q_DECLARE_TYPEINFO(NewMailNotificationHistoryManager::HistoryMailInfo, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(NewMailNotificationHistoryManager::HistoryFolderInfo, Q_RELOCATABLE_TYPE);
QDebug operator<<(QDebug d, const NewMailNotificationHistoryManager::HistoryFolderInfo &id);
QDebug operator<<(QDebug d, const NewMailNotificationHistoryManager::HistoryMailInfo &id);
