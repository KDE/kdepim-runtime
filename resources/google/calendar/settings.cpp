/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

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

#include "settings.h"
#include "settingsadaptor.h"

#include <KWallet/Wallet>

#include <QDBusConnection>

class SettingsHelper
{
public:
    SettingsHelper() : q(Q_NULLPTR)
    {
    }

    ~SettingsHelper()
    {
        delete q;
        q = Q_NULLPTR;
    }

    Settings *q;
};

Q_GLOBAL_STATIC(SettingsHelper, s_globalSettings)

Settings::Settings():
    GoogleSettings()
{
    Q_ASSERT(!s_globalSettings->q);
    s_globalSettings->q = this;

    new SettingsAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"), this,
            QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents);
}

Settings *Settings::self()
{
    if (!s_globalSettings->q) {
        new Settings;
        s_globalSettings->q->load();
    }

    return s_globalSettings->q;

}
