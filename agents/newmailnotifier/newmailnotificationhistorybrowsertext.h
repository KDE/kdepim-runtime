/*
    SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <TextCustomEditor/RichTextBrowser>

class NewMailNotificationHistoryBrowserText : public TextCustomEditor::RichTextBrowser
{
    Q_OBJECT
public:
    explicit NewMailNotificationHistoryBrowserText(QWidget *parent = nullptr);
    ~NewMailNotificationHistoryBrowserText() override;

protected:
    void doSetSource(const QUrl &url, QTextDocument::ResourceType type = QTextDocument::UnknownResource) override;

Q_SIGNALS:
    void clearHistory();
    void openMail(const QString &mailIdentifier);
    void openFolder(const QString &folderIdentifier);

protected:
    void addExtraMenuEntry(QMenu *menu, QPoint pos) override;

private:
    void slotOpenMail(const QString &identifier);
    void slotSelectFolder(const QString &identifier);
};
