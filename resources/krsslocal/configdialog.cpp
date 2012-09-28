/*
    This file is part of KRssLocal
    Copyright (c) 2012 Alessandro Cosentino <cosenal@gmail.com>

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

#include <KConfigDialogManager>
#include <KStandardDirs>
#include <KLineEdit>

ConfigDialog::ConfigDialog( QWidget *parent )
   : KDialog( parent )
{
  
    ui.setupUi( mainWidget() );
    m_manager = new KConfigDialogManager( this, Settings::self() );
    m_manager->updateWidgets();
    
    ui.urlPath->lineEdit()->setText( Settings::self()->path() );
    connect( this, SIGNAL(okClicked()), SLOT(slotSave()) );
    
    ui.kcfg_AutoFetchInterval->setSuffix(ki18np(" minute", "minutes"));
    connect( ui.kcfg_UseIntervalFetch, SIGNAL(toggled(bool)),
             ui.kcfg_AutoFetchInterval, SLOT(setEnabled(bool)) );
    connect( ui.kcfg_UseIntervalFetch, SIGNAL(toggled(bool)),
             ui.autoFetchIntervalLabel, SLOT(setEnabled(bool)) );

    if ( ui.kcfg_UseIntervalFetch->isChecked() ) {
        ui.kcfg_AutoFetchInterval->setEnabled( true );
        ui.autoFetchIntervalLabel->setEnabled( true );
    }
    
}
 
void ConfigDialog::slotSave()
{ 
    m_manager->updateSettings();

    const QString path = ui.urlPath->lineEdit()->text();
    Settings::self()->setPath( path );
}

#include "configdialog.moc"
 
