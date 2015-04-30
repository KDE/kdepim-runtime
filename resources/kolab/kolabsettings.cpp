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

#include <klocale.h>

KolabSettings::KolabSettings(WId winId) : Settings(winId)
{
    changeDefaults();
    load();
}

void KolabSettings::changeDefaults()
{
    setCurrentGroup(QLatin1String("network"));
    KConfigSkeleton::ItemInt  *itemImapPort;
    itemImapPort = (KConfigSkeleton::ItemInt *) findItem(QLatin1String("ImapPort"));
    itemImapPort->setDefaultValue(143);
    KConfigSkeleton::ItemString  *itemSafety;
    itemSafety = (KConfigSkeleton::ItemString *) findItem(QLatin1String("Safety"));
    itemSafety->setDefaultValue(QLatin1String("STARTTLS"));
    KConfigSkeleton::ItemBool  *itemSubscriptionEnabled;
    itemSubscriptionEnabled = (KConfigSkeleton::ItemBool *) findItem(QLatin1String("SubscriptionEnabled"));
    itemSubscriptionEnabled->setDefaultValue(true);

    setCurrentGroup(QLatin1String("cache"));
    KConfigSkeleton::ItemBool  *itemDisconnectedModeEnabled;
    itemDisconnectedModeEnabled = (KConfigSkeleton::ItemBool *) findItem(QLatin1String("DisconnectedModeEnabled"));
    itemDisconnectedModeEnabled->setDefaultValue(true);

    setCurrentGroup(QLatin1String("siever"));
    KConfigSkeleton::ItemBool  *itemSieveSupport;
    itemSieveSupport = (KConfigSkeleton::ItemBool *) findItem(QLatin1String("SieveSupport"));
    itemSieveSupport->setDefaultValue(true);
    KConfigSkeleton::ItemBool  *itemSieveReuseConfig;
    itemSieveReuseConfig = (KConfigSkeleton::ItemBool *) findItem(QLatin1String("SieveReuseConfig"));
    itemSieveReuseConfig->setDefaultValue(true);
}
