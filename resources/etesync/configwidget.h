/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include <QWidget>

class KConfigDialogManager;
class QLineEdit;
class QPushButton;
class Settings;
class ConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWidget(Settings *settings, QWidget *parent);

    void load();
    void save() const;

private:
    KConfigDialogManager *mManager = nullptr;
    QLineEdit *mServerEdit = nullptr;
    QLineEdit *mUserEdit = nullptr;
    QLineEdit *mPasswordEdit = nullptr;
    QLineEdit *mEncryptionPasswordEdit = nullptr;
};

#endif
