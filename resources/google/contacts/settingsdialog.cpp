/*
    Copyright (C) 2011-2013  Daniel Vrátil <dvratil@redhat.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "settingsdialog.h"
#include "settings.h"

#include <KDE/KWindowSystem>
#include <KDE/KLocalizedString>
#include <KDE/KDebug>

#include <LibKGAPI2/Account>
#include <LibKGAPI2/AuthJob>

using namespace KGAPI2;

SettingsDialog::SettingsDialog( GoogleAccountManager *accountMgr, WId windowId, GoogleResource *parent ):
    GoogleSettingsDialog( accountMgr, windowId, parent )
{
    connect( this, SIGNAL(accepted()),
             this, SLOT(saveSettings()) );
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::saveSettings()
{
    const AccountPtr account = currentAccount();
    if ( !account ) {
        Settings::self()->setAccount( QString() );
        Settings::self()->writeConfig();
        return;
    }

    Settings::self()->setAccount( account->accountName() );
    Settings::self()->writeConfig();
}
