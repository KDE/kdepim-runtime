/*
    SPDX-FileCopyrightText: 2009 Grégory Oestreicher <greg@kamago.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "ui_configwidget.h"

#include <KDAV/Enums>

#include <QDialog>

#include "settings.h"
#include <QList>
#include <QPair>
#include <QString>

class KConfigDialogManager;
class QStandardItemModel;

class ConfigWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param settings The settings
     * @param identifier The identifier of the resource
     * @param parent The parent widget
     */
    explicit ConfigWidget(Settings &settings, const QString &identifier, QWidget *parent = nullptr);
    ~ConfigWidget() override;

    void setPassword(const QString &password);

    void loadSettings();
    void saveSettings() const;

Q_SIGNALS:
    void okEnabled(bool enabled);

private:
    void onSyncRangeStartTypeChanged();
    void checkUserInput();
    void onAddButtonClicked();
    void onSearchButtonClicked();
    void onRemoveButtonClicked();
    void onEditButtonClicked();
    void checkConfiguredUrlsButtonsState();
    void onOkClicked();

    void addModelRow(const QString &protocol, const QString &url);
    void insertModelRow(int index, const QString &protocol, const QString &url);

    Settings &mSettings;
    const QString mIdentifier;
    Ui::ConfigWidget mUi;
    KConfigDialogManager *mManager = nullptr;
    QList<QPair<QString, KDAV::Protocol>> mAddedUrls;
    QList<QPair<QString, KDAV::Protocol>> mRemovedUrls;
    QStandardItemModel *const mModel;
};
