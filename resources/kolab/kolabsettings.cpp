/*
    Copyright (c) 2014 Sandro Knauß <knauss@kolabsys.com>

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

#include "kolabsettings.h"

KolabSettings::KolabSettings(WId winId) : Settings(winId)
{
    changeDefaults();
    load();
}

void KolabSettings::changeDefaults()
{
    setCurrentGroup(QStringLiteral("network"));
    KConfigSkeleton::ItemInt *itemImapPort = (KConfigSkeleton::ItemInt *)findItem(QStringLiteral("ImapPort"));
    itemImapPort->setDefaultValue(143);
    KConfigSkeleton::ItemString *itemSafety = (KConfigSkeleton::ItemString *)findItem(QStringLiteral("Safety"));
    itemSafety->setDefaultValue(QStringLiteral("STARTTLS"));
    KConfigSkeleton::ItemBool *itemSubscriptionEnabled = (KConfigSkeleton::ItemBool *)findItem(QStringLiteral("SubscriptionEnabled"));
    itemSubscriptionEnabled->setDefaultValue(true);

    setCurrentGroup(QStringLiteral("cache"));
    KConfigSkeleton::ItemBool *itemDisconnectedModeEnabled = (KConfigSkeleton::ItemBool *)findItem(QStringLiteral("DisconnectedModeEnabled"));
    itemDisconnectedModeEnabled->setDefaultValue(true);

    setCurrentGroup(QStringLiteral("siever"));
    KConfigSkeleton::ItemBool *itemSieveSupport = (KConfigSkeleton::ItemBool *)findItem(QStringLiteral("SieveSupport"));
    itemSieveSupport->setDefaultValue(true);
    KConfigSkeleton::ItemBool *itemSieveReuseConfig = (KConfigSkeleton::ItemBool *)findItem(QStringLiteral("SieveReuseConfig"));
    itemSieveReuseConfig->setDefaultValue(true);
}
