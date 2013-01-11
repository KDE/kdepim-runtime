/* Copyright 2010 Thomas McGuire <mcguire@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "ui_settingsdialog.h"

class FacebookResource;
class KJob;

class SettingsDialog : public KDialog, private Ui::SettingsDialog
{
  Q_OBJECT

  public:
    SettingsDialog( FacebookResource *parentResource, WId parentWindow );
    ~SettingsDialog();

  private slots:
    virtual void slotButtonClicked( int button );
    void resetAuthentication();
    void showAuthenticationDialog();
    void authenticationDone( const QString &accessToken );
    void authenticationCanceled();
    void userInfoJobDone( KJob *job );

  private:
    void setupWidgets();
    void loadSettings();
    void saveSettings();
    void updateAuthenticationWidgets();
    void updateUserName();

    FacebookResource *mParentResource;
    bool mTriggerSync;
};

#endif
