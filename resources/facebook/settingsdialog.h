/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SETTINGSDIALOG_H_
#define SETTINGSDIALOG_H_

#include <QDialog>
#include <QScopedPointer>

class FacebookResource;
class Ui_SettingsDialog;
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(FacebookResource *parent);
    ~SettingsDialog() override;

    private Q_SLOT:
    void save();
    void checkToken();
    void login();
    void logout();

private:
    FacebookResource *mResource = nullptr;
    QScopedPointer<Ui_SettingsDialog> ui;
};

#endif
