/*
    SPDX-FileCopyrightText: 2023-2026 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistorybrowsertext.h"
using namespace Qt::Literals::StringLiterals;

#include "newmailnotifier_debug.h"
#include "newmailnotifieropenfolderjob.h"
#include "newmailnotifiershowmessagejob.h"

#include <KStandardAction>
#include <QMenu>

NewMailNotificationHistoryBrowserText::NewMailNotificationHistoryBrowserText(QWidget *parent)
    : TextCustomEditor::RichTextBrowser(parent)
{
    connect(this, &NewMailNotificationHistoryBrowserText::openMail, this, &NewMailNotificationHistoryBrowserText::slotOpenMail);
    connect(this, &NewMailNotificationHistoryBrowserText::openFolder, this, &NewMailNotificationHistoryBrowserText::slotSelectFolder);
}

NewMailNotificationHistoryBrowserText::~NewMailNotificationHistoryBrowserText() = default;

void NewMailNotificationHistoryBrowserText::doSetSource(const QUrl &url, QTextDocument::ResourceType type)
{
    Q_UNUSED(type);
    constexpr auto openMailScheme = "openmail:"_L1;
    constexpr auto openFolderScheme = "openfolder:"_L1;

    QString uri = url.toString();
    qCDebug(NEWMAILNOTIFIER_LOG) << " uri " << uri;
    if (uri.startsWith(openMailScheme)) {
        uri.remove(0, openMailScheme.size());
        qCDebug(NEWMAILNOTIFIER_LOG) << "openMail uri " << uri;
        Q_EMIT openMail(uri);
    } else if (uri.startsWith(openFolderScheme)) {
        uri.remove(0, openFolderScheme.size());
        qCDebug(NEWMAILNOTIFIER_LOG) << "openFolder uri " << uri;
        Q_EMIT openFolder(uri);
    } else {
        qCWarning(NEWMAILNOTIFIER_LOG) << "No implement support for " << uri;
    }
}

void NewMailNotificationHistoryBrowserText::addExtraMenuEntry(QMenu *menu, [[maybe_unused]] QPoint pos)
{
    if (!toPlainText().isEmpty()) {
        QAction *clearAllAction = KStandardAction::clear(this, &NewMailNotificationHistoryBrowserText::clearHistory, menu);
        menu->addSeparator();
        menu->addAction(clearAllAction);
    }
}

void NewMailNotificationHistoryBrowserText::slotOpenMail(const QString &identifier)
{
    auto job = new NewMailNotifierShowMessageJob(identifier.toLong());
    job->start();
}

void NewMailNotificationHistoryBrowserText::slotSelectFolder(const QString &identifier)
{
    auto job = new NewMailNotifierOpenFolderJob(identifier);
    job->start();
}

#include "moc_newmailnotificationhistorybrowsertext.cpp"
