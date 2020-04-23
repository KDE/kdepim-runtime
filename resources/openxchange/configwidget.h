/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include <QWidget>

class KConfigDialogManager;
class KJob;
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

private Q_SLOTS:
    void updateButtonState();
    void checkConnection();
    void checkConnectionJobFinished(KJob *);

private:
    KConfigDialogManager *mManager = nullptr;
    QLineEdit *mServerEdit = nullptr;
    QLineEdit *mUserEdit = nullptr;
    QLineEdit *mPasswordEdit = nullptr;
    QPushButton *mCheckConnectionButton = nullptr;
};

#endif
