/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include "ui_settings.h"
class QPushButton;
class KConfigDialogManager;
class Settings;
class ConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigWidget(Settings *settings, QWidget *parent = nullptr);

    void load(Settings *settings);
    void save(Settings *settings) const;

Q_SIGNALS:
    void okEnabled(bool enabled);

private Q_SLOTS:
    void checkPath();

private:
    Ui::ConfigWidget ui;
    KConfigDialogManager *mManager = nullptr;
    QPushButton *mOkButton = nullptr;
    bool mToplevelIsContainer = false;
};

#endif
