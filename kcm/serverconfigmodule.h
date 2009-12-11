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

#ifndef SERVERCONFIGMODULE_H
#define SERVERCONFIGMODULE_H

#include "ui_serverconfigmodule.h"
#include "ui_servermysqlstorage.h"
#include "ui_serverpsqlstorage.h"
#include "ui_serverstoragedriver.h"

#include <KCModule>

class QComboBox;
class QStackedWidget;

class ServerConfigModule : public KCModule
{
  Q_OBJECT
  public:
    explicit ServerConfigModule( QWidget * parent, const QVariantList & args );

    void load();
    void save();
    void defaults();

  private slots:
    void updateStatus();
    void startStopClicked();
    void restartClicked();
    void selfTestClicked();
    void driverChanged(int);

  private:
    Ui::ServerConfigModule ui;
    Ui::StorageDriver ui_driver;
    Ui::MySQLStoragePage ui_mysql;
    Ui::PSQLStoragePage  ui_psql;

    QStackedWidget *m_stackWidget;
    QWidget *m_mysqlWidget;
    QWidget *m_psqlWidget;

    QComboBox *m_driverBox;
};

#endif
