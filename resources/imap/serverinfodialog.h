/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef SERVERINFODIALOG_H
#define SERVERINFODIALOG_H

#include <QDialog>
#include <QTextBrowser>
class ImapResourceBase;

class ServerInfoTextBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit ServerInfoTextBrowser(QWidget *parent = nullptr);
    ~ServerInfoTextBrowser() override;
protected:
    void paintEvent(QPaintEvent *event) override;
};

class ServerInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ServerInfoDialog(ImapResourceBase *parentResource, QWidget *parent);
    ~ServerInfoDialog();
private:
    ServerInfoTextBrowser *mTextBrowser = nullptr;
};

#endif // SERVERINFODIALOG_H
