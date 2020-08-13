/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include <QWidget>

class KConfigDialogManager;
class KJob;
class QLineEdit;
class QPushButton;
class Settings;
class ConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWidget(Settings *settings, QWidget *parent);

    void load();
    void save() const;

private Q_SLOTS:
    void updateButtonState();
    void checkConnection();
    void checkConnectionJobFinished(KJob *);

private:
    KConfigDialogManager *mManager = nullptr;
    QLineEdit *mServerEdit = nullptr;
    QLineEdit *mUserEdit = nullptr;
    QLineEdit *mPasswordEdit = nullptr;
    QPushButton *mCheckConnectionButton = nullptr;
};

#endif
