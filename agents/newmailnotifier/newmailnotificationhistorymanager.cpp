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
    return QStringLiteral(" <a href=\"%1\">%2</a>").arg(QStringLiteral("openFolder:%1").arg(id), i18n("[Open Folder]"));
}

QString NewMailNotificationHistoryManager::generateOpenMailStr(Akonadi::Item::Id id)
{
    return QStringLiteral(" <a href=\"%1\">%2</a>").arg(QStringLiteral("openMail:%1").arg(id), i18n("[Show Mail]"));
}

void NewMailNotificationHistoryManager::addEmailInfoNotificationHistory(const NewMailNotificationHistoryManager::HistoryMailInfo &info)
{
    // qDebug() << "NewMailNotificationHistoryManager::addFoldersInfoNotificationHistory  " << info;
    addHeader();
    QString messageInfo = info.message;
    cleanupStr(messageInfo);
    const QString message = messageInfo + generateOpenMailStr(info.identifier);
    mHistory += message;
    Q_EMIT historyAdded(mHistory.join(QStringLiteral("<br>")));
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
        QString messageInfo = info.message;
        cleanupStr(messageInfo);
        messages += messageInfo + generateOpenFolderStr(info.identifier);
    }
    mHistory += messages;
    Q_EMIT historyAdded(mHistory.join(QStringLiteral("<br>")));
}

void NewMailNotificationHistoryManager::setTestModeEnabled(bool test)
{
    mTestEnabled = test;
}

void NewMailNotificationHistoryManager::addHeader()
{
    if (!mHistory.isEmpty()) {
        mHistory += QStringLiteral("<br>");
    }
    if (mTestEnabled) { // Only for test
        mHistory += QStringLiteral("<b> %1 </b>").arg(QDate::currentDate().toString());
    } else {
        mHistory += QStringLiteral("<b> %1 </b>").arg(QDateTime::currentDateTime().toString());
    }
}

void NewMailNotificationHistoryManager::cleanupStr(QString &str)
{
    // str.replace(QStringLiteral("<br>"), QStringLiteral("\n"));
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
