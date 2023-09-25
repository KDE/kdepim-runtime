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
    return QStringLiteral(" <a href=\"%1\">%2</a>").arg(QStringLiteral("openMail:%1").arg(id), i18n("[Show Mail]"));
}

QString NewMailNotificationHistoryManager::generateOpenMailStr(Akonadi::Item::Id id)
{
    return QStringLiteral(" <a href=\"%1\">%2</a>").arg(QStringLiteral("openFolder:%1").arg(id), i18n("[Open Folder]"));
}

void NewMailNotificationHistoryManager::addEmailInfoNotificationHistory(const NewMailNotificationHistoryManager::HistoryMailInfo &info)
{
    // TODO addHeader();
    QString messageInfo = info.message;
    cleanupStr(messageInfo);
    const QString message = messageInfo + generateOpenMailStr(info.identifier);
    // mHistory += messages;
    // TODO
    // Q_EMIT historyAdded(newStr);
}

void NewMailNotificationHistoryManager::addFoldersInfoNotificationHistory(const QList<NewMailNotificationHistoryManager::HistoryFolderInfo> &infos)
{
    // TODO addHeader();
    QString messages;
    for (const NewMailNotificationHistoryManager::HistoryFolderInfo &info : infos) {
        if (!messages.isEmpty()) {
            messages += QStringLiteral("<br>");
        }
        QString messageInfo = info.message;
        cleanupStr(messageInfo);
        messages += messageInfo + generateOpenFolderStr(info.identifier);
        // TODO
    }
    // TODO
    // mHistory += messages;
    // Q_EMIT historyAdded(newStr);
}

void NewMailNotificationHistoryManager::addHeader()
{
#if 0
    if (!mHistory.isEmpty()) {
        mHistory += QStringLiteral("\n");
    }
    QString newStr = QStringLiteral("============ %1 ============").arg(QDateTime::currentDateTime().toString());
    if (!str.startsWith(QLatin1Char('\n')) && !str.startsWith(QStringLiteral("<br>"))) {
        newStr += QLatin1Char('\n');
    }
#endif
}

void NewMailNotificationHistoryManager::addHistory(QString str)
{
    if (!mHistory.isEmpty()) {
        mHistory += QStringLiteral("\n");
    }
    QString newStr = QStringLiteral("============ %1 ============").arg(QDateTime::currentDateTime().toString());
    if (!str.startsWith(QLatin1Char('\n')) && !str.startsWith(QStringLiteral("<br>"))) {
        newStr += QLatin1Char('\n');
    }
    cleanupStr(str);
    newStr += str;
    mHistory += newStr;
    Q_EMIT historyAdded(newStr);
}

void NewMailNotificationHistoryManager::cleanupStr(QString &str)
{
    str.replace(QStringLiteral("<br>"), QStringLiteral("\n"));
    str.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    str.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
}

void NewMailNotificationHistoryManager::setHistory(const QStringList &newHistory)
{
    mHistory = newHistory;
}

void NewMailNotificationHistoryManager::clear()
{
    mHistory.clear();
}

#include "moc_newmailnotificationhistorymanager.cpp"
