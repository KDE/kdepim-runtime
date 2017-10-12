/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef INFODIALOG_H
#define INFODIALOG_H

#include "kmigratorbase.h"
#include "migratorbase.h"
#include <QEventLoopLocker>
#include <QDialog>

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
    int mMigratorCount;
    static bool mError;
    bool mChange = false;
    bool mCloseWhenDone = false;
    bool mAutoScrollList = false;
};

#endif
