/*
    SPDX-FileCopyrightText: 2023-2026 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistorymanager.h"
#include <KLocalizedString>
#include <QDateTime>
#include <QDebug>
#include <QLocale>

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
    // qDebug() << "NewMailNotificationHistoryManager::addEmailInfoNotificationHistory  " << info;
    const QString message = info.message + generateOpenMailStr(info.identifier);
    appendEntry(header() + QStringLiteral("<br>") + message + QStringLiteral("<br>"));
}

void NewMailNotificationHistoryManager::addFoldersInfoNotificationHistory(const QList<NewMailNotificationHistoryManager::HistoryFolderInfo> &infos)
{
    // qDebug() << "NewMailNotificationHistoryManager::addFoldersInfoNotificationHistory  " << infos;
    QString messages;
    for (const NewMailNotificationHistoryManager::HistoryFolderInfo &info : infos) {
        if (!messages.isEmpty()) {
            messages += QStringLiteral("<br>");
        }
        messages += info.message + generateOpenFolderStr(info.identifier);
    }
    messages += QStringLiteral("<br>");
    appendEntry(header() + QStringLiteral("<br>") + messages);
}

void NewMailNotificationHistoryManager::setTestModeEnabled(bool test)
{
    mTestEnabled = test;
}

int NewMailNotificationHistoryManager::maximumHistorySize() const
{
    return mMaximumHistorySize;
}

void NewMailNotificationHistoryManager::setMaximumHistorySize(int size)
{
    mMaximumHistorySize = qMax(1, size);
    truncateHistory();
}

QString NewMailNotificationHistoryManager::header() const
{
    if (mTestEnabled) { // Only for test
        return QStringLiteral("<b> %1 </b>").arg(QDate::currentDate().toString());
    }
    return QStringLiteral("<b> %1 </b>").arg(QLocale().toString(QDateTime::currentDateTime()));
}

// One entry == one notification, so that dropping the oldest ones never splits a
// timestamp from the message it belongs to.
void NewMailNotificationHistoryManager::appendEntry(const QString &entry)
{
    mHistory.append(entry);
    truncateHistory();
    Q_EMIT historyAdded(joinHistory());
}

void NewMailNotificationHistoryManager::truncateHistory()
{
    while (mHistory.count() > mMaximumHistorySize) {
        mHistory.removeFirst();
    }
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
