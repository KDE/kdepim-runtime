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

    bool useDefaultCredentials() const;

    void setUsername(const QString &user);
    QString username() const;

    void setPassword(const QString &password);
    QString password() const;

    QStringList selection() const;

private:
    void checkUserInput();
    void search();
    void onSearchJobFinished(KJob *job);
    void onCollectionsFetchJobFinished(KJob *job);

    Ui::SearchDialog mUi;
    QStandardItemModel *mModel = nullptr;
    int mSubJobCount = 0;
};

#endif // SEARCHDIALOG_H
