/*
    Copyright (c) 2008 Bertjan Broeksema <b.broeksema@kdemail.org>
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

#ifndef AKONADI_SINGLEFILERESOURCECONFIGDIALOG_H
#define AKONADI_SINGLEFILERESOURCECONFIGDIALOG_H

#include "singlefileresourceconfigdialogbase.h"

#include <KDE/KConfigDialogManager>

namespace Akonadi {

/**
 * Configuration dialog for single file resources.
 */
template <typename Settings>
class SingleFileResourceConfigDialog : public SingleFileResourceConfigDialogBase
{
  public:
    explicit SingleFileResourceConfigDialog( WId windowId ) :
      SingleFileResourceConfigDialogBase( windowId )
    {
      ui.kcfg_Path->setUrl( KUrl( Settings::self()->path() ) );
      mManager = new KConfigDialogManager( this, Settings::self() );
      mManager->updateWidgets();
    }

  protected:
    void save()
    {
      mManager->updateSettings();
      Settings::self()->setPath( ui.kcfg_Path->url().url() );
      Settings::self()->writeConfig();
    }
};

}

#endif
