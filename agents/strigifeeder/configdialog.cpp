/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

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

#include "configdialog.h"

#include "settings.h"

#include <kconfigdialogmanager.h>
#include <kwindowsystem.h>

using namespace Akonadi_Strigifeeder_Agent;

ConfigDialog::ConfigDialog( WId windowId, Settings *settings, QWidget * parent )
  : KDialog( parent ),
    mSettings( settings )
{
  ui.setupUi( mainWidget() );
  setButtons( Ok | Cancel );

  if ( windowId )
    KWindowSystem::setMainWindow( this, windowId );

  connect( this, SIGNAL( okClicked() ), SLOT( save() ) );

  m_manager = new KConfigDialogManager( this, mSettings );
  m_manager->updateWidgets();
}


void ConfigDialog::save()
{
  m_manager->updateSettings();
  mSettings->writeConfig();
}

#include "configdialog.moc"
