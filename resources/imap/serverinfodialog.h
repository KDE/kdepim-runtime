/*
  SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include <QDialog>
#include <QTextBrowser>

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
    explicit ServerInfoDialog(const QString &identifier, QWidget *parent);
    ~ServerInfoDialog() override;

private:
    void writeConfig();
    void readConfig();
    ServerInfoTextBrowser *const mTextBrowser;
};
