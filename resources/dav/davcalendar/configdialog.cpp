/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "configdialog.h"
#include "settings.h"

#include <kconfigdialogmanager.h>

ConfigDialog::ConfigDialog( QWidget* p )
  : KDialog( p )
{
  ui.setupUi( mainWidget() );
  mManager = new KConfigDialogManager( this, Settings::self() );
  mManager->updateWidgets();
  
  connect( this, SIGNAL( okClicked() ), this, SLOT( onOkClicked() ) );
  connect( this, SIGNAL( cancelClicked() ), this, SLOT( onCancelClicked() ) );
  connect( ui.kcfg_remoteUrls, SIGNAL( removed( const QString& ) ), this, SLOT( urlRemoved( const QString& ) ) );
}

ConfigDialog::~ConfigDialog()
{
  delete mManager;
}

QStringList ConfigDialog::removedUrls() const
{
  return rmdUrls;
}

void ConfigDialog::setRemovedUrls( const QStringList &l )
{
  rmdUrls = l;
}

void ConfigDialog::onOkClicked()
{
  mManager->updateSettings();
  Settings::self()->setPassword( ui.kcfg_password->text() );
}

void ConfigDialog::onCancelClicked()
{
  rmdUrls.clear();
}

void ConfigDialog::urlRemoved( const QString &url )
{
  rmdUrls << url;
}

#include "configdialog.moc"