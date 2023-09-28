/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistorymanager.h"
#include <KLocalizedString>
#include <QDateTime>

NewMailNotificationHistoryManager::NewMailNotificationHistoryManager(QObject *parent)
    : QObject{parent}
{
}

NewMailNotificationHistoryManager::~NewMailNotificationHistoryManager() = default;

NewMailNotificationHistoryManager *NewMailNotificationHistoryManager::self()
{
    static NewMailNotificationHistoryManager s_self;
    return &s_self;
}

QStringList NewMailNotificationHistoryManager::history() const
{
    return mHistory;
}

QString NewMailNotificationHistoryManager::generateOpenFolderStr(Akonadi::Collection::Id id)
{
    return QStringLiteral(" <a href=\"%1\">%2</a>").arg(QStringLiteral("openfolder:%1").arg(id), i18n("[Open Folder]"));
}

QString NewMailNotificationHistoryManager::joinHistory() const
{
    return mHistory.join(QStringLiteral("<br>"));
}

QString NewMailNotificationHistoryManager::generateOpenMailStr(Akonadi::Item::Id id)
{
    return QStringLiteral(" <a href=\"%1\">%2</a>").arg(QStringLiteral("openmail:%1").arg(id), i18n("[Show Mail]"));
}

void NewMailNotificationHistoryManager::addEmailInfoNotificationHistory(const NewMailNotificationHistoryManager::HistoryMailInfo &info)
{
    // qDebug() << "NewMailNotificationHistoryManager::addFoldersInfoNotificationHistory  " << info;
    addHeader();
    const QString messageInfo = info.message;
    const QString message = messageInfo + generateOpenMailStr(info.identifier);
    mHistory += message;
    Q_EMIT historyAdded(joinHistory());
}

void NewMailNotificationHistoryManager::addFoldersInfoNotificationHistory(const QList<NewMailNotificationHistoryManager::HistoryFolderInfo> &infos)
{
    // qDebug() << "NewMailNotificationHistoryManager::addFoldersInfoNotificationHistory  " << infos;
    addHeader();
    QString messages;
    for (const NewMailNotificationHistoryManager::HistoryFolderInfo &info : infos) {
        if (!messages.isEmpty()) {
            messages += QStringLiteral("<br>");
        }
        const QString messageInfo = info.message;
        messages += messageInfo + generateOpenFolderStr(info.identifier);
    }
    mHistory += messages;
    Q_EMIT historyAdded(joinHistory());
}

void NewMailNotificationHistoryManager::setTestModeEnabled(bool test)
{
    mTestEnabled = test;
}

void NewMailNotificationHistoryManager::addHeader()
{
    // if (!mHistory.isEmpty()) {
    //     mHistory += QStringLiteral("<br>");
    // }
    if (mTestEnabled) { // Only for test
        mHistory += QStringLiteral("<b> %1 </b>").arg(QDate::currentDate().toString());
    } else {
        mHistory += QStringLiteral("<b> %1 </b>").arg(QDateTime::currentDateTime().toString());
    }
}

void NewMailNotificationHistoryManager::setHistory(const QStringList &newHistory)
{
    mHistory = newHistory;
}

void NewMailNotificationHistoryManager::clear()
{
    mHistory.clear();
}

QDebug operator<<(QDebug d, const NewMailNotificationHistoryManager::HistoryFolderInfo &id)
{
    d << "message: " << id.message;
    d << "identifier: " << id.identifier;
    return d;
}
QDebug operator<<(QDebug d, const NewMailNotificationHistoryManager::HistoryMailInfo &id)
{
    d << "message: " << id.message;
    d << "identifier: " << id.identifier;
    return d;
}

#include "moc_newmailnotificationhistorymanager.cpp"
