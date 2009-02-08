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

#include "akonadiconfigmodule.h"

#include <kpluginfactory.h>
#include <kpluginloader.h>

K_PLUGIN_FACTORY( AkonadiConfigModuleFactory, registerPlugin<AkonadiConfigModule>(); )
K_EXPORT_PLUGIN( AkonadiConfigModuleFactory( "kcm_akonadi" ) )

AkonadiConfigModule::AkonadiConfigModule( QWidget * parent, const QVariantList & args ) :
    KCModuleContainer( parent )
{
  Q_UNUSED( args );
  setButtons( KCModule::Default | KCModule::Apply );
  addModule( QLatin1String("kcm_akonadi_resources") );
  addModule( QLatin1String("kcm_akonadi_server") );
}

