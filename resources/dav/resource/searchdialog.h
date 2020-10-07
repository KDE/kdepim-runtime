/*
 *  SPDX-FileCopyrightText: 2011 Grégory Oestreicher <greg@kamago.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

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

    Q_REQUIRED_RESULT bool useDefaultCredentials() const;

    void setUsername(const QString &user);
    Q_REQUIRED_RESULT QString username() const;

    void setPassword(const QString &password);
    Q_REQUIRED_RESULT QString password() const;

    Q_REQUIRED_RESULT QStringList selection() const;

private:
    void checkUserInput();
    void search();
    void onSearchJobFinished(KJob *job);
    void onCollectionsFetchJobFinished(KJob *job);

    Ui::SearchDialog mUi;
    QStandardItemModel *const mModel;
    int mSubJobCount = 0;
};

#endif // SEARCHDIALOG_H
