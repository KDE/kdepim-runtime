/*
 * Copyright 2013  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef MIGRATIONSTATUSWIDGET_H
#define MIGRATIONSTATUSWIDGET_H

#include "migrationscheduler.h"
#include <QWidget>
#include <QItemSelectionModel>

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
