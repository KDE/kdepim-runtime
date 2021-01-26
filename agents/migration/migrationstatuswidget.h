/*
 * SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#ifndef MIGRATIONSTATUSWIDGET_H
#define MIGRATIONSTATUSWIDGET_H

#include "migrationscheduler.h"
#include <QItemSelectionModel>
#include <QWidget>

class MigrationStatusWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MigrationStatusWidget(MigrationScheduler &scheduler, QWidget *parent = nullptr);

public Q_SLOTS:
    void onItemActivated(const QModelIndex &);

private Q_SLOTS:
    void startSelected();
    void pauseSelected();
    void abortSelected();

private:
    MigrationScheduler &mScheduler;
    QItemSelectionModel *mSelectionModel = nullptr;
};

#endif // MIGRATIONCONFIGDIALOG_H
