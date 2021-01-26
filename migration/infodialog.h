/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef INFODIALOG_H
#define INFODIALOG_H

#include "kmigratorbase.h"
#include "migratorbase.h"
#include <QDialog>
#include <QEventLoopLocker>

class QLabel;
class QListWidget;
class QProgressBar;
class QDialogButtonBox;

class InfoDialog : public QDialog
{
    Q_OBJECT
public:
    InfoDialog(bool closeWhenDone = true);
    ~InfoDialog();

    static bool hasError()
    {
        return mError;
    }

public Q_SLOTS:
    void message(KMigratorBase::MessageType type, const QString &msg);
    void message(MigratorBase::MessageType type, const QString &msg);

    void migratorAdded();
    void migratorDone();

    void status(const QString &msg);

    void progress(int value);
    void progress(int min, int max, int value);

private:
    bool hasChange() const
    {
        return mChange;
    }

    void scrollBarMoved(int value);
    QEventLoopLocker eventLoopLocker;
    QDialogButtonBox *mButtonBox = nullptr;
    QListWidget *mList = nullptr;
    QLabel *mStatusLabel = nullptr;
    QProgressBar *mProgressBar = nullptr;
    int mMigratorCount = 0;
    static bool mError;
    bool mChange = false;
    const bool mCloseWhenDone;
    bool mAutoScrollList = true;
};

#endif
