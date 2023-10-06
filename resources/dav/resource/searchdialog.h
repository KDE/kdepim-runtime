/*
 *  SPDX-FileCopyrightText: 2011 Grégory Oestreicher <greg@kamago.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "ui_searchdialog.h"

#include <QDialog>

class KJob;
class QStandardItemModel;

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog(QWidget *parent = nullptr);
    ~SearchDialog() override;

    [[nodiscard]] bool useDefaultCredentials() const;

    void setUsername(const QString &user);
    [[nodiscard]] QString username() const;

    void setPassword(const QString &password);
    [[nodiscard]] QString password() const;

    [[nodiscard]] QStringList selection() const;

private:
    void checkUserInput();
    void search();
    void onSearchJobFinished(KJob *job);
    void onCollectionsFetchJobFinished(KJob *job);

    Ui::SearchDialog mUi;
    QStandardItemModel *const mModel;
    int mSubJobCount = 0;
};
