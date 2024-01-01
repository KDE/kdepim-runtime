/*
    SPDX-FileCopyrightText: 2023-2024 Laurent Montel <montel.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistorybrowsertext.h"
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
    QString uri = url.toString();
    // qDebug() << " uri " << uri;
    if (uri.startsWith(QLatin1String("openmail:"))) {
        uri.remove(QStringLiteral("openmail:"));
        // qDebug() << "openMail uri " << uri;
        Q_EMIT openMail(uri);
    } else if (uri.startsWith(QLatin1String("openfolder:"))) {
        uri.remove(QStringLiteral("openfolder:"));
        // qDebug() << "openFolder uri " << uri;
        Q_EMIT openFolder(uri);
    }
}

void NewMailNotificationHistoryBrowserText::addExtraMenuEntry(QMenu *menu, QPoint pos)
{
    Q_UNUSED(pos)
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
